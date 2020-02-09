/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

/* Support for Sockets. */
#include <net/socket.h>

/* Support for WiFi. */
#include <net/wifi_mgmt.h>
#include <net/net_event.h>

/* Support for TFTP. */
#include <net/tftp.h>

/* Hardcoded SSID / Password for now. Should be updated by the user per their config. */
#define MY_SSID     "GalaxyJ7"
#define PASS        "bilalwasim"

/* Hardcoded values for Server IP / Port. Should be updated by the user per their config. */
#define TFTPS_IP     "192.168.43.138"
#define TFTPS_PORT   51000

/* User buffer and structure for getting / putting data from / to the TFTP Server. */
static u8_t  buf[16384];
struct tftpc tmp_struct = {
	.user_buf      = (u8_t *) buf, /* Pointer to user buffer. */
	.user_buf_size = 16384         /* Size of the buffer. */
};


/* Function: WiFi_Setup
 * Description: This function tries to connect with a given SSID / Password pair.  */
static int32_t WiFi_Setup(void)
{
	/* Get the default network interface (which will be WiFi in our configuration). */
	struct net_if *iface = net_if_get_default();
	struct wifi_connect_req_params cnx_params = {0};

	/* Populate defaults. */
	cnx_params.ssid        = MY_SSID;
	cnx_params.ssid_length = strlen(MY_SSID);
	cnx_params.psk         = PASS;
	cnx_params.psk_length  = strlen(PASS);
	cnx_params.channel     = WIFI_CHANNEL_ANY;
	cnx_params.security    = WIFI_SECURITY_TYPE_PSK;

	/* Connect with the default "GalaxyJ7" SSID. */
	net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &cnx_params, sizeof(struct wifi_connect_req_params));

	/* Wait until connection is established with the given SSID. */
	return (net_mgmt_event_wait(NET_EVENT_WIFI_CONNECT_RESULT, NULL, &iface, NULL, NULL, K_FOREVER));
}

void main(void)
{
	int                ret;
	struct sockaddr_in server;

	/* Fill in the server info.
	 * - Use INET.
	 * - Use UDP Port  - TFTPS_PORT.
	 * - Use Server IP - TFTPS_IP. */
	server.sin_family = AF_INET;
	server.sin_port   = htons(TFTPS_PORT);
	inet_pton(AF_INET, TFTPS_IP, &server.sin_addr);

	/* Log message. */
	printk("Hello World! %s\n", CONFIG_BOARD);

	/* Lets setup WiFi. */
	WiFi_Setup();
	
	/* Lets get data from the TFTP Server. */
	ret = tftp_get(&server, &tmp_struct, "test", "octet");

	while (1);
}
