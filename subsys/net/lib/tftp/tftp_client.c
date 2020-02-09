/*
 * Copyright (c) 2020 bwasim
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(tftp_client, LOG_LEVEL_DBG);

/* Public / Internal TFTP header file. */
#include <net/tftp.h>
#include "tftp_client.h"

/* TFTP Request Buffer / Size variables. */
static u8_t   tftpc_request_buffer[TFTPC_MAX_BUF_SIZE];
static u32_t  tftpc_request_size;

/* TFTP Request block number property. */
static u32_t  tftpc_block_no;
static u32_t  tftpc_index;

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
static inline int tftpc_send_ack(int sock, int block) {

	u8_t tmp[4];

	LOG_INF("Client acking Block Number: %d", block);

	/* Fill in the "Ack" Opcode and the block no. */
	fillshort(tmp, ACK_OPCODE);
	fillshort(tmp + 2, block);

	/* Lets send this request buffer out. Size of request buffer is 4 bytes. */
	return send(sock, tmp, 4, 0);
}

/* Name: tftpc_send_err
 * Description: This function sends an Error report to the TFTP Server. */
static inline int tftpc_send_err(int sock, int err_code, char *err_string) {

	LOG_ERR("Client Error. Sending code: %d(%s)", err_code, err_string);

	/* Fill in the "Err" Opcode and the actual error code. */
	fillshort(tftpc_request_buffer, ERROR_OPCODE);
	fillshort(tftpc_request_buffer + 2, err_code);
	tftpc_request_size = 4;

	/* Copy the Error String. */
	strcpy(tftpc_request_buffer + 4, err_string);
	tftpc_request_size += strlen(err_string);

	/* Lets send this request buffer out. Size of request buffer is 4 bytes. */
	return send(sock, tftpc_request_buffer, tftpc_request_size, 0);
}

/* Name: tftpc_recv
 * Description: This function tries to get data from the TFTP Server (either response
 * or data). Times out eventually. */
static int tftpc_recv(int sock) {

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
static int tftpc_process(int sock, struct tftpc *client) {

	u16_t    block_no;
	u32_t    cpy_size;
	bool     send_ack = true;

	/* Get the block number as received in the packet. */
	block_no = getshort(tftpc_request_buffer + 2);

	/* Is this the block number we are looking for? */
	if (block_no == tftpc_block_no) {

		LOG_INF("Server sent Block Number: %d", tftpc_block_no);

		/* OK - This is the block number we are looking for. Lets store this data in some
		 * user buffer (or file).
		 * -> Note that the first 4 bytes of the "response" is the TFTP header. Everything else is the
		 * user buffer.
		 * -> Before pushing data to the buffer, we should check if we have overflowed? If so, we can
		 * probably store some of this data and exit after sending an error to the server. Or we can simply
		 * exit without storing any data. Both approaches are okay. Here, we've implemented the first one. */
		if ((tftpc_request_size - TFTP_HEADER_SIZE) >
				(client->user_buf_size - tftpc_index)) {
			/* Can't copy all. Lets only copy a minimal set and set an error. */
			cpy_size = client->user_buf_size - tftpc_index;
			send_ack = false;
		}
		else {
			/* Can copy all. */
			cpy_size = tftpc_request_size - TFTP_HEADER_SIZE;
		}

		/* Perform the actual copy and update the index. */
		memcpy(client->user_buf + tftpc_index,
				tftpc_request_buffer + TFTP_HEADER_SIZE, cpy_size);
		tftpc_index += cpy_size;

		/* Was there any problem? */
		if (send_ack == true) {

			/* Now we are in a position to ack this data. */
			tftpc_send_ack(sock, block_no);
		}
		else {

			/* Send Error :-( */
			tftpc_send_err(sock, 0x3, "Buffer Overflow");

			/* Log the error. */
			LOG_ERR("For the transfer of this file, the user provided buffer of size: %d", client->user_buf_size);
			LOG_ERR("However, it seems that the actual file has larger size. Exiting !");

			return (TFTPC_BUFFER_OVERFLOW);
		}

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

/* Name: tftp_send_request
 * Description: This function sends out a request to the TFTP Server (Read / Write) and waits for a response.
 * Once we get some response from the server, it is interpreted and ensured to be correct. If not, we keep on
 * poking the server for data until we eventually give up.  */
static int tftp_send_request(int sock, u8_t request,
		                     const char *remote_file,  const char *mode) {

	u8_t    no_of_retransmists = 0;
	s32_t   stat;
	u16_t   server_response    = -1;

	/* Socket connection successfully - Create the Read Request Packet (RRQ). */
	make_request(remote_file, mode, request);

	do {

		/* Send this request to the TFTP Server. */
		stat = send(sock, tftpc_request_buffer, tftpc_request_size, 0);

		/* Data sent successfully? */
		if (stat > 0) {

			/* We have been able to send the Read Request to the TFTP Server. The server will respond
			 * to our message with data or error, And we need to handle these cases correctly.
			 *
			 * Lets try to get this data from the TFTP Server. */
			stat = tftpc_recv(sock);

			/* Lets check if we were able to get response from the server and if the response was Ok or not. */
			if (stat > 0) {

				/* Ok - We were able to get response of our read request from the TFTP Server.
				 * Lets check and see what the TFTP Server has to say about our request. */
				server_response = getshort(tftpc_request_buffer);

				/* Did we get some err? */
				if (server_response == ERROR_OPCODE) {

					/* The server responded with some errors here. Lets get to know about the specific
					 * error and log it. Nothing else we can do here really and so should exit. */
					LOG_ERR("tftp_get failure - Server returned: %d", getshort(tftpc_request_buffer + 2));

					break;
				}

				/* Did we get some data? */
				if (server_response == DATA_OPCODE) {

					/* Good News - TFTP Server responded with data. Lets talk to the server and
					 * get all data. */
					LOG_DBG("tftp_get success - Server returned: %d", getshort(tftpc_request_buffer + 2));

					break;
				}
			}
		}

		/* No of times we had to re-tx this "request". */
		no_of_retransmists ++;

		/* Log this out. */
		LOG_DBG("tftp_send_request was either unable to get data from the TFTP Server.");
		LOG_DBG("Or failed to get any valid data.");
		LOG_INF("no_of_retransmists = %d", no_of_retransmists);
		LOG_INF("Are we re-transmitting: %s", (no_of_retransmists < TFTP_REQ_RETX) ? "yes": "no");

	} while (no_of_retransmists < TFTP_REQ_RETX);

	/* Status? */
	return (server_response);
}

/* Name: tftp_get
 * Description: This function gets "file" from the remote server. */
int tftp_get(struct sockaddr_in *server, struct tftpc *client,
		     const char *remote_file, const char *mode) {
	int     stat;
	int     sock;
	u16_t   server_response;
	u8_t    no_of_retransmists = 0;

	/* Re-init the global "block number" variable. */
	tftpc_block_no = 1;
	tftpc_index    = 0;

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

	/* Send out the request to the TFTP Server. */
	server_response = tftp_send_request(sock, RRQ_OPCODE, remote_file, mode);

	do {
		/* Did the server respond with data? */
		if (server_response == DATA_OPCODE) {

			/* Good News - TFTP Server responded with data. Lets talk to the server and
			 * get all data. */
			stat = tftpc_process(sock, client);

			/* Lets get more data if data was successful or we got a duplicate packet. */
			if ((stat == TFTPC_DATA_RX_SUCCESS) || (stat == TFTPC_DUPLICATE_DATA)) {

recv:
				/* Receive data from the server. */
				stat = tftpc_recv(sock);

				/* There are two possibilities at this point.
				 * - We got the data successfully.
				 * - We timed out trying to get data.
				 * The first case is Ok, but the second case needs special handling. Like before,
				 * if we time out getting data, we will re-tx. However, in this case, we will re-tx
				 * ack of the previous block. */
				if (stat > 0) {

					/* Log. */
					LOG_INF("Recevied data of size: %d", stat);

					/* Response? */
					server_response = getshort(tftpc_request_buffer);
				}
				else {

					/* Log. */
					LOG_INF("Timed out while receiving data from the server: %d", stat);

					/* Re-TX update. */
					no_of_retransmists ++;

					/* Should we re-tx? */
					if (no_of_retransmists < TFTP_REQ_RETX) {

						LOG_INF("Re-transmisting the ack and waiting for data again. ");

						/* Bad News - We timed out. Lets send out an ack of the previous block. */
						tftpc_send_ack(sock, tftpc_block_no);

						goto recv;
					}
					else {
						LOG_ERR("No more retransmits available. Exiting");
						break;
					}
				}
			}
		}
		else if (server_response == ERROR_OPCODE) {
			/* The server responded with some errors here. Lets get to know about the specific
			 * error and log it. Nothing else we can do here really and so should exit. */
			LOG_ERR("tftp_get failure - Server returned: %d", getshort(tftpc_request_buffer + 2));

			break;
		}
	} while (stat > 0);

	/* Close out this socket. */
	close(sock);

	/* Stat? */
	return (stat);
}

/*
 * int tftp_put(struct tftpc *props, const char *remote_file, const char *local_file) {
 *	return 0;
 * }
 */
