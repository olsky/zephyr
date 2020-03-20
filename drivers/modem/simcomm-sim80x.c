/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(SIM800, CONFIG_MODEM_LOG_LEVEL);

/* SIM driver header file. */
#include "simcomm-sim80x.h"

/* Static data to bind the interface driver with Modem Context. */
static struct modem_data    mdata;
static struct modem_context mctx;
static struct modem_iface   iface;

/* Static data to bind the interface driver with attentive library. */
static struct cellular *sim800_modem;
static struct at       *sim800_at;

/* Modem Interface */
static struct modem_iface_uart_data iface_data;
static u8_t   iface_isr_buf[MDM_RECV_BUF_SIZE];
static u8_t   iface_rb_buf[MDM_MAX_DATA_LENGTH];


/* Externing global function to allocate an instance of Zephyr AT. */
extern struct at       *at_alloc_zephyr(void);

/* Modem pin definition. */
static struct modem_pin modem_pins[] = {
	/* MDM_RESET */
	MODEM_PIN(DT_INST_0_SIMCOMM_SIM80X_MDM_RESET_GPIOS_CONTROLLER,
			DT_INST_0_SIMCOMM_SIM80X_MDM_RESET_GPIOS_PIN, GPIO_DIR_OUT),
};

/**
 * @brief  Convert string to long integer, but handle errors
 *
 * @param  s: string with representation of integer number
 * @param  err_value: on error return this value instead
 * @param  desc: name the string being converted
 * @param  func: function where this is called (typically __func__)
 *
 * @retval return integer conversion on success, or err_value on error
 */
static int modem_atoi(const char *s, const int err_value,
		      const char *desc, const char *func)
{
	int ret;
	char *endptr;

	ret = (int)strtol(s, &endptr, 10);
	if (!endptr || *endptr != '\0') {
		LOG_ERR("bad %s '%s' in %s", log_strdup(s), log_strdup(desc),
			log_strdup(func));
		return err_value;
	}

	return ret;
}

/* send binary data via the +USO[ST/WR] commands */
static int send_socket_data(struct modem_socket *sock,
			    const struct sockaddr *dst_addr,
			    const char *buf, size_t buf_len, int timeout)
{
	int ret;

	if (!sock) {
		return -EINVAL;
	}

	/* UDP is not supported. */
	if (sock->ip_proto == IPPROTO_UDP) {
		LOG_ERR("UDP is not supported by simcomm-sim80x driver");
		return -EPFNOSUPPORT;
	}

	/* Ask attentive to write this out! */
	ret = sim800_modem->ops->socket_send(sim800_modem, (sock->id - 7), buf, buf_len, 0);

	/* Op completed successfully? */
	if (ret == buf_len) {
		LOG_INF("data sent with len=<%d>", buf_len);
	} else {
		LOG_ERR("data sent failure");
	}

	return ret;
}

/* Handler: +USOCR: <socket_id>[0] */
MODEM_CMD_DEFINE(on_cmd_sockcreate)
{
	struct modem_socket *sock = NULL;

	/* look up new socket by special id */
	sock = modem_socket_from_newid(&mdata.socket_config);
	if (sock) {
		sock->id = ATOI(argv[0],
				mdata.socket_config.base_socket_num - 1,
				"socket_id");
		/* on error give up modem socket */
		if (sock->id == mdata.socket_config.base_socket_num - 1) {
			modem_socket_put(&mdata.socket_config, sock->sock_fd);
		}
	}

	/* don't give back semaphore -- OK to follow */
}

/* Function: simcomm_sim80x_dev_get
 * Description: Get the Modem device. */
struct cellular * simcomm_sim80x_dev_get(void) {
	return (sim800_modem);
}

/*
 * generic socket creation function
 * which can be called in bind() or connect()
 */
static int create_socket(struct modem_socket *sock, const struct sockaddr *addr)
{
	int ret;
	struct modem_cmd cmd = MODEM_CMD("+USOCR: ", on_cmd_sockcreate, 1U, "");
	char buf[sizeof("AT+USOCR=#,#####\r")];
	u16_t local_port = 0U, proto = 6U;

	/* UDP is not supported. */
	if (sock->ip_proto == IPPROTO_UDP) {
		LOG_ERR("UDP is not supported by simcomm-sim80x driver");
		return -EPFNOSUPPORT;
	}

	if (addr) {
		if (addr->sa_family == AF_INET6) {
			local_port = ntohs(net_sin6(addr)->sin6_port);
		} else if (addr->sa_family == AF_INET) {
			local_port = ntohs(net_sin(addr)->sin_port);
		}
	}

	if (local_port > 0U) {
		snprintk(buf, sizeof(buf), "AT+USOCR=%d,%u", proto, local_port);
	} else {
		snprintk(buf, sizeof(buf), "AT+USOCR=%d", proto);
	}

	/* create socket */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     &cmd, 1U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		modem_socket_put(&mdata.socket_config, sock->sock_fd);
	}

	return ret;
}

/*
 * Socket Offload OPS
 */

static int offload_socket(int family, int type, int proto)
{
	/* defer modem's socket create call to bind() */
	return modem_socket_get(&mdata.socket_config, family, type, proto);
}

static int offload_close(int sock_fd)
{
	struct modem_socket *sock;
	int ret;

	sock = modem_socket_from_fd(&mdata.socket_config, sock_fd);
	if (!sock) {
		/* socket was already closed?  Exit quietly here. */
		return 0;
	}

	/* make sure we assigned an id */
	if (sock->id < mdata.socket_config.base_socket_num) {
		return 0;
	}

	/* Attentive to close out this socket. */
    ret = sim800_modem->ops->socket_close(sim800_modem, (sock->id - 7));

    /* Was attentive successful? */
	if (ret < 0) {
		LOG_ERR("socket_close ret:%d", ret);
	}

	modem_socket_put(&mdata.socket_config, sock_fd);
	return 0;
}

static int offload_bind(int sock_fd, const struct sockaddr *addr,
			socklen_t addrlen)
{
	struct modem_socket *sock = NULL;

	sock = modem_socket_from_fd(&mdata.socket_config, sock_fd);
	if (!sock) {
		LOG_ERR("Can't locate socket from fd:%d", sock_fd);
		return -EINVAL;
	}

	/* save bind address information */
	memcpy(&sock->src, addr, sizeof(*addr));

	/* make sure we've created the socket */
	if (sock->id == mdata.socket_config.sockets_len + 1) {
		return create_socket(sock, addr);
	}

	return 0;
}

static int offload_connect(int sock_fd, const struct sockaddr *addr,
			   socklen_t addrlen)
{
	struct modem_socket *sock;
	int ret;
	u16_t dst_port = 0U;

	if (!addr) {
		return -EINVAL;
	}

	sock = modem_socket_from_fd(&mdata.socket_config, sock_fd);
	if (!sock) {
		LOG_ERR("Can't locate socket from fd:%d", sock_fd);
		return -EINVAL;
	}

	if (sock->id < mdata.socket_config.base_socket_num - 1) {
		LOG_ERR("Invalid socket_id(%d) from fd:%d",
			sock->id, sock_fd);
		return -EINVAL;
	}

	/* UDP is not supported. */
	if (sock->ip_proto == IPPROTO_UDP) {
		LOG_ERR("UDP is not supported by simcomm-sim80x driver");
		return -EPFNOSUPPORT;
	}

	/* Copy all data necessary. */
	memcpy(&sock->dst, addr, sizeof(*addr));
	if (addr->sa_family == AF_INET6) {
		dst_port = ntohs(net_sin6(addr)->sin6_port);
	} else if (addr->sa_family == AF_INET) {
		dst_port = ntohs(net_sin(addr)->sin_port);
	} else {
		return -EPFNOSUPPORT;
	}

	/* Invoke the Attentive Library to create this connection. */
	ret = sim800_modem->ops->socket_connect(sim800_modem, (sock->id - 7),
			                         modem_context_sprint_ip_addr(addr), dst_port);

	if (ret < 0) {
		LOG_ERR("socket_connect request for addr: %s, port: %d failed", modem_context_sprint_ip_addr(addr), dst_port);
	}

	return ret;
}

/* support for POLLIN only for now. */
static int offload_poll(struct pollfd *fds, int nfds, int msecs)
{
	int ret = modem_socket_poll(&mdata.socket_config, fds, nfds, msecs);

	if (ret < 0) {
		LOG_ERR("ret:%d errno:%d", ret, errno);
	}

	return ret;
}

static ssize_t offload_recvfrom(int sock_fd, void *buf, short int len,
				short int flags, struct sockaddr *from,
				socklen_t *fromlen)
{
	struct modem_socket *sock;
	short int read = 0;

	if (!buf || len == 0) {
		return -EINVAL;
	}

	sock = modem_socket_from_fd(&mdata.socket_config, sock_fd);
	if (!sock) {
		LOG_ERR("Can't locate socket from fd:%d", sock_fd);
		return -EINVAL;
	}

	int timeout = 3;

	do
	{
		/* TODO: We should keep a per-socket buffer and store data in that buffer. This is because
		 * when we request data from the SIM80x module, it can return more data than required.
		 * The application consumes the required data and the rest is lost. This should NOT
		 * happen.
		 *
		 * See the SARA-R4 for reference. What does it do? */

		k_sleep(K_MSEC(50));

		/* Ask attentive to give us the data. */
		read += sim800_modem->ops->socket_recv(sim800_modem, (sock->id-7), buf, len, 0);

		timeout --;

		/* If we have the required data, lets exit with no error. */
		if (read == len) {
			break;
		}

		/* If we have timed out, lets exit with timeout error. */
		if (timeout == 0) {
			errno = EAGAIN; read = -1;
			break;
		}

	} while (true);

	LOG_INF("SIM800: recvfrom got %d data", read);

	/* clear socket data */
	sock->data = NULL;
	return (read);
}

static ssize_t offload_recv(int sock_fd, void *buf, size_t max_len, int flags)
{
	return offload_recvfrom(sock_fd, buf, (short int)max_len, flags,
				NULL, NULL);
}

static ssize_t offload_sendto(int sock_fd, const void *buf, size_t len,
			      int flags, const struct sockaddr *to,
			      socklen_t tolen)
{
	struct modem_socket *sock;

	if (!buf || len == 0) {
		return -EINVAL;
	}

	sock = modem_socket_from_fd(&mdata.socket_config, sock_fd);
	if (!sock) {
		LOG_ERR("Can't locate socket from fd:%d", sock_fd);
		return -EINVAL;
	}

	/* UDP is not supported. */
	if (sock->ip_proto == IPPROTO_UDP) {
		LOG_ERR("UDP is not supported by simcomm-sim80x driver");
		return -EPFNOSUPPORT;
	}

	return send_socket_data(sock, to, buf, len, MDM_CMD_TIMEOUT);
}

static ssize_t offload_send(int sock_fd, const void *buf, size_t len, int flags)
{
	return offload_sendto(sock_fd, buf, len, flags, NULL, 0U);
}

static const struct socket_offload modem_socket_offload = {
	.socket = offload_socket,
	.close = offload_close,
	.bind = offload_bind,
	.connect = offload_connect,
	.poll = offload_poll,
	.recv = offload_recv,
	.recvfrom = offload_recvfrom,
	.send = offload_send,
	.sendto = offload_sendto,
};

static int net_offload_dummy_get(sa_family_t family,
				 enum net_sock_type type,
				 enum net_ip_protocol ip_proto,
				 struct net_context **context)
{

	LOG_ERR("NET_SOCKET_OFFLOAD must be configured for this driver");

	return -ENOTSUP;
}

/* placeholders, until Zepyr IP stack updated to handle a NULL net_offload */
static struct net_offload modem_net_offload = {
	.get = net_offload_dummy_get,
};

#define HASH_MULTIPLIER		37
static u32_t hash32(char *str, int len)
{
	u32_t h = 0;
	int i;

	for (i = 0; i < len; ++i) {
		h = (h * HASH_MULTIPLIER) + str[i];
	}

	return h;
}

static inline u8_t *modem_get_mac(struct device *dev)
{
	struct modem_data *data = dev->driver_data;
	u32_t hash_value;

	data->mac_addr[0] = 0x00;
	data->mac_addr[1] = 0x10;

	/* use IMEI for mac_addr */
	hash_value = hash32(mdata.mdm_imei, strlen(mdata.mdm_imei));

	UNALIGNED_PUT(hash_value, (u32_t *)(data->mac_addr + 2));

	return data->mac_addr;
}

static void modem_net_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct modem_data *data = dev->driver_data;

	/* Direct socket offload used instead of net offload: */
	iface->if_dev->offload = &modem_net_offload;
	net_if_set_link_addr(iface, modem_get_mac(dev),
			     sizeof(data->mac_addr),
			     NET_LINK_ETHERNET);
	socket_offload_register(&modem_socket_offload);
	data->net_iface = iface;
}

static struct net_if_api api_funcs = {
	.init = modem_net_iface_init,
};

static int modem_setup(void)
{
	/* Setup the structures. */
	iface_data.isr_buf = &iface_isr_buf[0];
	iface_data.isr_buf_len = sizeof(iface_isr_buf);
	iface_data.rx_rb_buf = &iface_rb_buf[0];
	iface_data.rx_rb_buf_len = sizeof(iface_rb_buf);

	/* Use the Modem Interface to setup UART. */
	modem_iface_uart_init(&iface, &iface_data, DT_INST_0_SIMCOMM_SIM80X_BUS_NAME);

	/* Context assignments. */
	mctx.iface             = iface;
	mctx.pins              = modem_pins;
	mctx.pins_len          = ARRAY_SIZE(modem_pins);
	mctx.data_manufacturer = mdata.mdm_manufacturer;
	mctx.data_model        = mdata.mdm_model;
	mctx.data_revision     = mdata.mdm_revision;
	mctx.data_imei         = mdata.mdm_imei;
	mctx.driver_data       = &mdata;

	/* Register the Modem. */
	int ret = modem_context_register(&mctx);

	/* Modem registered successfully? */
	if (ret == 0)
	{
		/* Assert Reset. */
		LOG_INF("MDM_RESET_PIN -> ASSERTED");
		modem_pin_write(&mctx, MDM_RESET, MDM_RESET_ASSERTED);
		k_sleep(K_SECONDS(1));

		/* De-assert Reset. */
		LOG_INF("MDM_RESET_PIN -> NOT_ASSERTED");
		modem_pin_write(&mctx, MDM_RESET, MDM_RESET_NOT_ASSERTED);

		/* Wait for modem to stabilize. */
		LOG_INF("Waiting for Modem to stabilize");
		k_sleep(K_SECONDS(8));
	}

	/* status ? */
	return (ret);
}

static int modem_init(struct device *dev)
{
	int ret = -1;

	ARG_UNUSED(dev);

	/* Init semaphore. */
	k_sem_init(&mdata.sem_response, 0, 1);

	/* Socket config */
	mdata.socket_config.sockets = &mdata.sockets[0];
	mdata.socket_config.sockets_len = ARRAY_SIZE(mdata.sockets);
	mdata.socket_config.base_socket_num = MDM_BASE_SOCKET_NUM;

	/* Socket Initialization. */
	ret = modem_socket_init(&mdata.socket_config);

	/* Init successful? */
	if (ret >= 0) {

		/* Allocate an instance of the SIM800 modem. */
	    sim800_modem = cellular_sim800_alloc();

	    /* Valid Modem Instance? */
	    if (sim800_modem != NULL) {

		    /* Allocate an instance of the AT hardware device. */
		    sim800_at    = at_alloc_zephyr();

		    /* Valid "AT" Instance? */
		    if (sim800_at != NULL) {

		    	/* Perform modem setup. */
		    	modem_setup();

		    	/* AT layer needs info about UART. */
		    	sim800_at->arg = (void *) &iface;

		    	/* Open this "AT" device. */
		    	if (at_open(sim800_at) == 0) {

			    	/* Attach Modem with "AT" instance. */
			    	ret = cellular_attach(sim800_modem, sim800_at, CONFIG_MODEM_SIM80X_APN);

					/* Get the IMEI. */
					(void) sim800_modem->ops->imei(sim800_modem,
							               (char *) mdata.mdm_imei, MDM_IMEI_LENGTH);

					/* Get the manufacturer. */
					(void) sim800_modem->ops->manufacturer(sim800_modem,
							               (char *) mdata.mdm_manufacturer, MDM_MANUFACTURER_LENGTH);

					/* Get the model. */
					(void) sim800_modem->ops->model(sim800_modem,
							               (char *) mdata.mdm_model, MDM_MODEL_LENGTH);

					/* Get the revision. */
					(void) sim800_modem->ops->model(sim800_modem,
							               (char *) mdata.mdm_revision, MDM_REVISION_LENGTH);

					/* Get the RSSI. */
					mctx.data_rssi = sim800_modem->ops->rssi(sim800_modem);
		    	}
		    }
	    }
	}

	/* Modem Initialized successfully? */
	return (ret);
}

/* Create the SIM80x device. */
NET_DEVICE_OFFLOAD_INIT(modem_sara, CONFIG_MODEM_SIM80X_NAME,
						modem_init, &mdata, NULL,
						CONFIG_MODEM_SIM80X_INIT_PRIORITY, &api_funcs,
						MDM_MAX_DATA_LENGTH);
