/*
 * Copyright (c) 2020 bwasim
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(tftp_client, LOG_LEVEL_DBG);

/* TFTP Client standard header file. */
#include "tftp_client.h"

/* TFTP Request Buffer / Size variables. */
u8_t   tftpc_request_buffer[TFTPC_MAX_BUF_SIZE];
u32_t  tftpc_request_size;

/* TFTP Request block number property. */
static u32_t  tftpc_block_no;

u8_t   user_buf[1024];
u32_t  user_buf_index = 0;

/* Global timeval structure indicating the timeout interval for all TFTP transactions. */
static struct timeval tftpc_timeout = {
	.tv_sec   =   CONFIG_TFTPC_REQUEST_TIMEOUT,
	.tv_usec  =   0
};

/* Number of simultaneous client connections will be NUM_FDS be minus 2 */
fd_set readfds;
fd_set workfds;
int fdnum;

static int pollfds_add(int fd ) {

	FD_SET(fd, &readfds);

	if (fd >= fdnum) {
		fdnum = fd + 1;
	}

	return 0;
}

/* Name: fillshort
 * Description: This function fills the provided u16_t buffer into a u8_t buffer array. */
static inline void fillshort(u8_t *buf, u16_t data) {
	buf[0] = (data >> 8) & 0xFF;
	buf[1] = (data >> 0) & 0xFF;
}

/* Name: getshort
 * Description: This function gets u16_t buffer into a u8_t buffer array. */
static inline u16_t getshort(u8_t *buf) {
	u16_t   tmp;

	/* Create this short number. */
	tmp   = buf[0] << 8;
	tmp   = buf[1];

	/* Return it to the caller. */
	return (tmp);
}

/* Name: fillchar
 * Description: This function fills the provided u8_t buffer into a u8_t buffer array. */
static inline void fillchar(u8_t * buf, u8_t data) {
	buf[0] = data;
}

/* Name: make_rrq
 * Description: This function takes in a given list of parameters and returns a read request packet.
 *              This packet can be sent out directly to the TFTP server. */
static inline void make_request(const char *remote_file, const char *mode, u8_t request_type) {

	/* Populate the read request with the provided params. Note that this is created
	 * per RFC1350. */

	/* Read Request? */
	if (request_type == RRQ_OPCODE) {

		/* Fill in the "Read" Opcode. Also keep tabs on the request size. */
		fillshort(tftpc_request_buffer, RRQ_OPCODE);
		tftpc_request_size = 2;
	}

	/* Copy in the name of the remote file, the file we need to get from the server.
	 * Add an upper bound to ensure no buffers overflow. */
	strncpy(tftpc_request_buffer + tftpc_request_size, remote_file, TFTP_MAX_FILENAME_SIZE);
	tftpc_request_size += strlen(remote_file);

	/* Fill in 0. */
	fillchar(tftpc_request_buffer + tftpc_request_size, 0x0);
	tftpc_request_size ++;

	/* Copy the mode of operation. For now, we only support "Octet" and the user should ensure that
	 * this is the case. Otherwise we will run into problems. */
	strncpy(tftpc_request_buffer + tftpc_request_size, mode, TFTP_MAX_MODE_SIZE);
	tftpc_request_size += strlen(mode);

	/* Fill in 0. */
	fillchar(tftpc_request_buffer + tftpc_request_size, 0x0);
	tftpc_request_size ++;
}

/* Name: make_wrq
 * Description: This function takes in a given list of parameters and returns a write request packet.
 *              This packet can be sent out directly to the TFTP server. */
static inline void make_wrq(char *request, const char *remote_file, const char *mode) {
}

/* Name: tftpc_send_ack
 * Description: This function sends an Ack to the TFTP Server (in response to the data sent by the
 * Server). */
int tftpc_send_ack(int sock, int block) {

	u8_t tmp[4];

	LOG_INF("Client acking Block Number: %d", block);

	/* Fill in the "Ack" Opcode and the block no. */
	fillshort(tmp, ACK_OPCODE);
	fillshort(tmp + 2, block);

	/* Lets send this request buffer out. Size of request buffer is 4 bytes. */
	return send(sock, tmp, 4, 0);
}

/* Name: tftpc_recv
 * Description: This function tries to get data from the TFTP Server (either response
 * or data). Times out eventually. */
int tftpc_recv(int sock) {

	int     stat;

	/* Enable select on the socket. */
	pollfds_add(sock);

	/* Enable select on this socket. Wait for the specified number of seconds
	 * before exiting the call. */
	stat = select(fdnum, &readfds, NULL, NULL, &tftpc_timeout);

	/* Did we get some data? Or did we time out? */
	if (stat > 0) {

		/* Receive data from the TFTP Server. */
		stat = recv(sock, tftpc_request_buffer, TFTPC_MAX_BUF_SIZE, 0);

		/* Store the amount of data received in the global variable. */
		tftpc_request_size = stat;
	}

	return (stat);
}

/* Name: tftpc_process
 * Description: This function will process the data received from the TFTP Server (a file or part of the file)
 * and place it in the user buffer. */
int tftpc_process(int sock) {

	u16_t block_no;

	/* Get the block number as received in the packet. */
	block_no = getshort(tftpc_request_buffer + 2);

	/* Is this the block number we are looking for? */
	if (block_no == tftpc_block_no) {

		LOG_INF("Server sent Block Number: %d", tftpc_block_no);

		/* OK - This is the block number we are looking for. Lets store this data in some
		 * user buffer (or file).
		 *
		 * Note that the first 4 bytes of the "response" is the TFTP header. Everything else is the
		 * user buffer. */
		memcpy(user_buf + user_buf_index, tftpc_request_buffer + TFTP_HEADER_SIZE,
				         tftpc_request_size - TFTP_HEADER_SIZE);

		/* User buffer index to be updated. */
		user_buf_index += (tftpc_request_size - TFTP_HEADER_SIZE);

		/* Now we are in a position to ack this data. */
		tftpc_send_ack(sock, block_no);

		/* We recevied a "block" of data. Lets update the global book-keeping variable that
		 * tracks the number of blocks received. */
		tftpc_block_no ++;

		/* Because we are RFC1350 compliant, the block size will always be 512. If it is equal to 512,
		 * we will assume that the transfer is still in progress. If we have block size less than 512,
		 * we will conclude the transfer. */
		return (tftpc_request_size - TFTP_HEADER_SIZE == TFTP_BLOCK_SIZE) ?
				(TFTPC_DATA_RX_SUCCESS) : (TFTPC_OP_COMPLETED);
	}
	else if (tftpc_block_no > block_no) {

		LOG_INF("Server send duplicate (already acked) block again. Block Number: %d", block_no);
		LOG_INF("Client already trying to receive Block Number: %d", tftpc_block_no);

		/* OK - This means that we just received a block of data that has already been received and
		 * acked by the TFTP client. In this case, we don't need to re-get or re-ack the data. */
		return (TFTPC_DUPLICATE_DATA);
	}

	/* Don't know what's going on. */
	return (TFTPC_UNKNOWN_FAILURE);
}

/* Name: tftp_get
 * Description: This function gets "file" from the remote server. */
int tftp_get(struct sockaddr_in *server, const char *remote_file,
		     const char *local_file, const char *mode) {
	int     stat;
	int     sock;

	/* Re-init the global "block number" variable. */
	tftpc_block_no = 1;

	/* Create Socket for TFTP. Use IPv4 / UDP as L4 / L3 communication layers. */
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	/* Valid sock descriptor? If not, best to tell the caller about it. */
	if (sock < 0) {
		LOG_ERR("Failed to create UDP socket (IPv4): %d", errno);
		return -errno;
	}

	/* Connect with the address.  */
	stat = connect(sock, (struct sockaddr *) server, sizeof(struct sockaddr_in));
	if (stat < 0) {
		LOG_ERR("Cannot connect to UDP remote (IPv4): %d", errno);
		return -errno;
	}

	/* Socket connection successfully - Create the Read Request Packet (RRQ). */
	make_request(remote_file, mode, RRQ_OPCODE);

	/* Send this request to the TFTP Server. */
	stat = send(sock, tftpc_request_buffer, tftpc_request_size, 0);

	/* Data sent successfully? */
	if (stat > 0) {

		do {
			/* We have been able to send the Read Request to the TFTP Server. The server will respond
			 * to our message with data or error, And we need to handle these cases correctly.
			 *
			 * Lets try to get this data from the TFTP Server. */
			stat = tftpc_recv(sock);

			/* Were we able to receive data successfully? Or did we timeout? */
			if (stat > 0) {

				/* Ok - We were able to get response of our read request from the TFTP Server.
				 * Lets check and see what the TFTP Server has to say about our request. */
				u16_t server_response = getshort(tftpc_request_buffer);

				/* Did we get some data? */
				if (server_response == DATA_OPCODE) {

					/* Good News - TFTP Server responded with data. Lets talk to the server and
					 * get all data. */
					stat = tftpc_process(sock);
				}

				/* Did we get some err? */
				if (server_response == ERROR_OPCODE) {

					/* The server responded with some errors here. Lets get to know about the specific
					 * error and log it. Nothing else we can do here really and so should exit. */
					LOG_ERR("tftp_get failure - Server returned: %d", getshort(tftpc_request_buffer + 2));
				}
			}
		} while (stat != TFTPC_OP_COMPLETED);
	}

	/* Lets close out this socket before returning. */
	return (stat = close(sock));
}

/*
 * int tftp_put(struct tftpc *props, const char *remote_file, const char *local_file) {
 *	return 0;
 * }
 */
