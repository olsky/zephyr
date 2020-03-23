/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board_lights.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(mqtt_publisher, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <net/socket.h>
#include <net/mqtt.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "mqtt_publisher.h"

#define MAX_IMEI_LEN (30)
#define CLIENT_ID_LEN (MAX_IMEI_LEN + 6)
static u8_t client_id_buf[CLIENT_ID_LEN+1];
static u8_t publish_topic[MAX_IMEI_LEN+128];

/* Buffers for MQTT client. */
static u8_t rx_buffer[APP_MQTT_BUFFER_SIZE];
static u8_t tx_buffer[APP_MQTT_BUFFER_SIZE];

#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
/* Making RX buffer large enough that the full IPv6 packet can fit into it */
#define MQTT_LIB_WEBSOCKET_RECV_BUF_LEN 1280

/* Websocket needs temporary buffer to store partial packets */
static u8_t temp_ws_rx_buf[MQTT_LIB_WEBSOCKET_RECV_BUF_LEN];
#endif

/* The mqtt client struct */
static struct mqtt_client client_ctx;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

#if defined(CONFIG_SOCKS)
static struct sockaddr socks5_proxy;
#endif

static struct pollfd fds[1];
static int nfds;

static bool connected;

#if defined(CONFIG_MQTT_LIB_TLS)

#include "test_certs.h"

#define TLS_SNI_HOSTNAME "localhost"
#define APP_CA_CERT_TAG 1
#define APP_PSK_TAG 2


static sec_tag_t m_sec_tags[] = {
#if defined(MBEDTLS_X509_CRT_PARSE_C) || defined(CONFIG_NET_SOCKETS_OFFLOAD)
		APP_CA_CERT_TAG,
#endif
#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
		APP_PSK_TAG,
#endif
};

static int tls_init(void)
{
	int err = -EINVAL;

#if defined(MBEDTLS_X509_CRT_PARSE_C) || defined(CONFIG_NET_SOCKETS_OFFLOAD)
	err = tls_credential_add(APP_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate, sizeof(ca_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register public certificate: %d", err);
		return err;
	}
#endif

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	err = tls_credential_add(APP_PSK_TAG, TLS_CREDENTIAL_PSK,
				 client_psk, sizeof(client_psk));
	if (err < 0) {
		LOG_ERR("Failed to register PSK: %d", err);
		return err;
	}

	err = tls_credential_add(APP_PSK_TAG, TLS_CREDENTIAL_PSK_ID,
				 client_psk_id, sizeof(client_psk_id) - 1);
	if (err < 0) {
		LOG_ERR("Failed to register PSK ID: %d", err);
	}
#endif

	return err;
}

#endif /* CONFIG_MQTT_LIB_TLS */

static void prepare_fds(struct mqtt_client *client)
{
	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds[0].fd = client->transport.tcp.sock;
	}
#if defined(CONFIG_MQTT_LIB_TLS)
	else if (client->transport.type == MQTT_TRANSPORT_SECURE) {
		fds[0].fd = client->transport.tls.sock;
	}
#endif

	fds[0].events = ZSOCK_POLLIN;
	nfds = 1;
}

static void clear_fds(void)
{
	nfds = 0;
}

static void wait(int timeout)
{
	if (nfds > 0) {
		if (poll(fds, nfds, timeout) < 0) {
			LOG_ERR("poll error: %d", errno);
		}
	}
}


subscribe_cb_t m_subscribe_cb = NULL;

static void mqtt_evt_handler(struct mqtt_client *const client,
			const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			printk("MQTT connect failed %d\n", evt->result);
			break;
		}

		connected = true;
		printk("MQTT client connected!\n");

		break;

	case MQTT_EVT_DISCONNECT:
		printk("MQTT client disconnected %d\n", evt->result);

		connected = false;
		clear_fds();

		break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			printk("MQTT PUBACK error %d\n", evt->result);
			break;
		}

		LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);

		break;

	case MQTT_EVT_PUBREC:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBREC error %d", evt->result);
			break;
		}

		LOG_INF("PUBREC packet id: %u", evt->param.pubrec.message_id);

		const struct mqtt_pubrel_param rel_param = {
			.message_id = evt->param.pubrec.message_id
		};

		err = mqtt_publish_qos2_release(client, &rel_param);
		if (err != 0) {
			LOG_ERR("Failed to send MQTT PUBREL: %d", err);
		}

		break;

	case MQTT_EVT_PUBCOMP:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBCOMP error %d", evt->result);
			break;
		}

		LOG_INF("PUBCOMP packet id: %u",
			evt->param.pubcomp.message_id);

		break;

	case MQTT_EVT_PUBLISH:
		if (evt->result != 0) {
			printk("MQTT PUBLISH error %d\n", evt->result);
			break;
		}

		LOG_INF("PUBLISH packet id: %u", evt->param.publish.message_id);

		u8_t d[33];
		int len = evt->param.publish.message.payload.len;
		int bytes_read;

		LOG_INF("MQTT publish received %d, %d bytes",
			  evt->result, len);
		LOG_INF("   id: %d, qos: %d",
			  evt->param.publish.message_id,
			  evt->param.publish.message.topic.qos);

		/* assuming the config message is textual */
		while (len) {
			bytes_read = mqtt_read_publish_payload(
							&client_ctx, d,
							len >= 32 ? 32 : len);

			if (bytes_read < 0 && bytes_read != -EAGAIN) {
				LOG_ERR("failure to read payload");
				break;
			}

			d[bytes_read] = '\0';
			LOG_INF("   payload: %s", log_strdup(d));
			if (m_subscribe_cb)
				m_subscribe_cb(d);

			len -= bytes_read;
		}

		struct mqtt_puback_param puback;

		puback.message_id = evt->param.publish.message_id;
		mqtt_publish_qos1_ack(&client_ctx, &puback);

		break;

	default:
		LOG_INF("MQTT event received %d", evt->type);
		break;
	}
}

static int publish(struct mqtt_client *client, char *payload)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = MQTT_QOS;
	param.message.topic.topic.utf8 = publish_topic;
	param.message.topic.topic.size =
			strlen(param.message.topic.topic.utf8);
	param.message.payload.data = payload;
	param.message.payload.len =
			strlen(param.message.payload.data);
	param.message_id = sys_rand32_get();
	param.dup_flag = 0U;
	param.retain_flag = 0U;

	return mqtt_publish(client, &param);
}

#define RC_STR(rc) ((rc) == 0 ? "OK" : "ERROR")

#define PRINT_RESULT(func, rc) \
	LOG_INF("%s: %d <%s>", (func), rc, RC_STR(rc))

static void broker_init(void)
{
#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 *broker6 = (struct sockaddr_in6 *)&broker;

	broker6->sin6_family = AF_INET6;
	broker6->sin6_port = htons(SERVER_PORT);
	inet_pton(AF_INET6, SERVER_ADDR, &broker6->sin6_addr);

#if defined(CONFIG_SOCKS)
	struct sockaddr_in6 *proxy6 = (struct sockaddr_in6 *)&socks5_proxy;

	proxy6->sin6_family = AF_INET6;
	proxy6->sin6_port = htons(SOCKS5_PROXY_PORT);
	inet_pton(AF_INET6, SOCKS5_PROXY_ADDR, &proxy6->sin6_addr);
#endif
#else
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &broker4->sin_addr);
#if defined(CONFIG_SOCKS)
	struct sockaddr_in *proxy4 = (struct sockaddr_in *)&socks5_proxy;

	proxy4->sin_family = AF_INET;
	proxy4->sin_port = htons(SOCKS5_PROXY_PORT);
	inet_pton(AF_INET, SOCKS5_PROXY_ADDR, &proxy4->sin_addr);
#endif
#endif
}

static int init_client_id(void)
{
	const char* imei = "0123456789";

	printk("innblue > %s [%d] > enter\n", __func__, __LINE__);

	// FIXME: This code needs to be updated to get the IMEI.

	snprintf(publish_topic, sizeof(publish_topic),
		     CONFIG_MQTT_PUB_TOPIC, imei);
	printk("\tpub-topic for this gateway: %s\n", publish_topic);

	return 0;
}

static void client_init(struct mqtt_client *client)
{
	mqtt_client_init(client);

	broker_init();

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	// will get it in the initialize...
	client->client_id.utf8 = (u8_t *)client_id_buf; //MQTT_CLIENTID;
	client->client_id.size = strlen(client_id_buf); //MQTT_CLIENTID);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* MQTT transport configuration */
#if defined(CONFIG_MQTT_LIB_TLS)
#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
	client->transport.type = MQTT_TRANSPORT_SECURE_WEBSOCKET;
#else
	client->transport.type = MQTT_TRANSPORT_SECURE;
#endif

	struct mqtt_sec_config *tls_config = &client->transport.tls.config;

	tls_config->peer_verify = 2;
	tls_config->cipher_list = NULL;
	tls_config->sec_tag_list = m_sec_tags;
	tls_config->sec_tag_count = ARRAY_SIZE(m_sec_tags);
#if defined(MBEDTLS_X509_CRT_PARSE_C) || defined(CONFIG_NET_SOCKETS_OFFLOAD)
	tls_config->hostname = TLS_SNI_HOSTNAME;
#else
	tls_config->hostname = NULL;
#endif

#else
#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
	client->transport.type = MQTT_TRANSPORT_NON_SECURE_WEBSOCKET;
#else
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif
#endif

#if defined(CONFIG_MQTT_LIB_WEBSOCKET)
	client->transport.websocket.config.host = SERVER_ADDR;
	client->transport.websocket.config.url = "/mqtt";
	client->transport.websocket.config.tmp_buf = temp_ws_rx_buf;
	client->transport.websocket.config.tmp_buf_len =
						sizeof(temp_ws_rx_buf);
	client->transport.websocket.timeout = K_SECONDS(5);
#endif

#if defined(CONFIG_SOCKS)
	mqtt_client_set_proxy(client, &socks5_proxy,
			      socks5_proxy.sa_family == AF_INET ?
			      sizeof(struct sockaddr_in) :
			      sizeof(struct sockaddr_in6));
#endif
}

static bool mqtt_sub(void)
{
	LOG_INF("try to subscribe to %s", CONFIG_MQTT_SUB_TOPIC);
	struct mqtt_topic subscribe_topics[] = {
		{
			.topic = {
				.utf8 = CONFIG_MQTT_SUB_TOPIC,
				.size = strlen(CONFIG_MQTT_SUB_TOPIC)
			},
			.qos = MQTT_QOS_1_AT_LEAST_ONCE
		}
	};

	const struct mqtt_subscription_list subscription_list = {
		.list = subscribe_topics,
		.list_count = ARRAY_SIZE(subscribe_topics),
		.message_id = 1U
	};

	const int rc = mqtt_subscribe(&client_ctx, &subscription_list);
	PRINT_RESULT("try_to_subscribe", rc);
	return 0 == rc;
}



/* In this routine we block until the connected variable is 1 */
static int try_to_connect(struct mqtt_client *client)
{
	int rc, i = 0;

	while (i++ < APP_CONNECT_TRIES && !connected) {

		client_init(client);

		rc = mqtt_connect(client);
		if (rc != 0) {
			PRINT_RESULT("mqtt_connect", rc);
			k_sleep(APP_SLEEP_MSECS);
			continue;
		}

		prepare_fds(client);

		wait(APP_SLEEP_MSECS);
		mqtt_input(client);

		if (!connected) {
			mqtt_abort(client);
		}
	}

	if (connected) {
		return 0;
	}

	return -EINVAL;
}

static int process_mqtt_and_sleep(struct mqtt_client *client, int timeout)
{
	s64_t remaining = timeout;
	s64_t start_time = k_uptime_get();
	int rc;

	while (remaining > 0 && connected) {
		wait(remaining);

		rc = mqtt_live(client);
		if (rc != 0) {
			PRINT_RESULT("mqtt_live", rc);
			return rc;
		}

		rc = mqtt_input(client);
		if (rc != 0) {
			PRINT_RESULT("mqtt_input", rc);
			return rc;
		}

		remaining = timeout + start_time - k_uptime_get();
	}

	return 0;
}

#define SUCCESS_OR_RETURN(rc) { if (rc != 0) { return rc; } }
#define SUCCESS_OR_BREAK(rc) { if (rc != 0) { return; } }

static bool initialize(void)
{
	LOG_INF("initialize mqtt...");
	// get mqtt client id from -->
	return 0 == init_client_id();
}

static bool process_input()
{
	if (!connected) {
		board_lights.modem_connected(false);
		int rc;

#if defined(CONFIG_MQTT_LIB_TLS)
		rc = tls_init();
		PRINT_RESULT("tls_init", rc);
#endif
		LOG_INF("attempting to connect: ");
		rc = try_to_connect(&client_ctx);
		PRINT_RESULT("try_to_connect", rc);
		if (0 != rc) {
			LOG_INF("could not connect to mqtt!!!");
			return false;
		}
		// connected here, do subscribe
		if (!mqtt_sub())
			return false;
		board_lights.modem_connected(true);
	}

	return 0 == process_mqtt_and_sleep(&client_ctx, APP_SLEEP_MSECS);
}

static bool publish_msg (char *message)
{
	LOG_INF("try to publish mqtt_publish: %s", message);
	const int rc = publish(&client_ctx, message);
	PRINT_RESULT("mqtt_publish", rc);
	return 0 == rc;
}

static bool set_subscribe_callback(subscribe_cb_t subscribe_cb)
{
	if (!subscribe_cb)
		return false;

	m_subscribe_cb = subscribe_cb;

	return true;
}

const mqtt_publisher_t mqtt_publisher = {
	.initialize = initialize,
	.publish = publish_msg,
	.set_subscribe_callback = set_subscribe_callback,
	.process_input = process_input
};
