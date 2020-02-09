/*
 * Copyright (c) 2020 bwasim
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TFTP_CLIENT_H_
#define TFTP_CLIENT_H_

/* Generic header files required by the TFTPC Module. */
#include <zephyr.h>
#include <errno.h>
#include <net/socket.h>

/* Defines for creating static arrays for TFTP communication. */
#define TFTP_MAX_FILENAME_SIZE   CONFIG_TFTPC_MAX_FILENAME_SIZE
#define TFTP_HEADER_SIZE         4
#define TFTP_BLOCK_SIZE          512
#define TFTP_MAX_MODE_SIZE       8
#define TFTP_REQ_RETX            CONFIG_TFTPC_REQUEST_RETRANSMITS

/* Maximum amount of data that can be received in a single go ! */
#define TFTPC_MAX_BUF_SIZE       (TFTP_BLOCK_SIZE + TFTP_HEADER_SIZE)

/* TFTP Opcodes. */
#define RRQ_OPCODE               0x1
#define WRQ_OPCODE               0x2
#define DATA_OPCODE              0x3
#define ACK_OPCODE               0x4
#define ERROR_OPCODE             0x5

/* TFTP Client Success / Error Codes. */
#define TFTPC_DATA_RX_SUCCESS     1
#define TFTPC_DUPLICATE_DATA     -1
#define TFTPC_UNKNOWN_FAILURE    -100
#define TFTPC_OP_COMPLETED        2

#endif /* #define TFTP_CLIENT_H_ */
