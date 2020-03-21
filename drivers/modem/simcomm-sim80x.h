/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SIMCOMM_SIM80X_H_
#define _SIMCOMM_SIM80X_H_

#include <kernel.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <device.h>
#include <init.h>

#include <net/net_if.h>
#include <net/net_offload.h>
#include <net/socket_offload.h>

#include "modem_context.h"
#include "modem_socket.h"
#include "modem_cmd_handler.h"
#include "modem_iface_uart.h"

/* Attentive Library. */
#include <at.h>
#include <cellular.h>

/* Generic defines for Modem. */
#define MDM_CMD_TIMEOUT			K_SECONDS(10)
#define MDM_MAX_DATA_LENGTH		1024
#define MDM_RECV_BUF_SIZE		128
#define MDM_MAX_SOCKETS			6
#define MDM_BASE_SOCKET_NUM		0
#define MDM_MANUFACTURER_LENGTH		10
#define MDM_MODEL_LENGTH		16
#define MDM_REVISION_LENGTH		64
#define MDM_IMEI_LENGTH			16
#define RSSI_TIMEOUT_SECS		30
#define MDM_MAX_DATA_LENGTH		1024
#define MDM_RECV_MAX_BUF		30

/* Number of retries to configure the SIM. */
#define SIM800_CIPCFG_RETRIES           10

/* helper macro to keep readability */
#define ATOI(s_, value_, desc_) modem_atoi(s_, value_, desc_, __func__)

/* pin settings */
enum mdm_control_pins {
	MDM_RESET = 0,
};

/* driver data */
struct modem_data {
	struct net_if *net_iface;
	u8_t mac_addr[6];

	/* modem interface */
	struct modem_iface_uart_data iface_data;
	u8_t iface_isr_buf[MDM_RECV_BUF_SIZE];
	u8_t iface_rb_buf[MDM_MAX_DATA_LENGTH];

	/* modem cmds */
	struct modem_cmd_handler_data cmd_handler_data;
	u8_t cmd_read_buf[MDM_RECV_BUF_SIZE];
	u8_t cmd_match_buf[MDM_RECV_BUF_SIZE];

	/* socket data */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];

	/* modem data */
	char mdm_manufacturer[MDM_MANUFACTURER_LENGTH];
	char mdm_model[MDM_MODEL_LENGTH];
	char mdm_revision[MDM_REVISION_LENGTH];
	char mdm_imei[MDM_IMEI_LENGTH];

	/* modem state */
	int ev_creg;

	/* response semaphore */
	struct k_sem sem_response;
};

#endif /* _SIMCOMM_SIM80X_H_ */
