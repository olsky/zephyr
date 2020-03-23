/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <net/mqtt.h>


#ifdef CONFIG_NET_CONFIG_SETTINGS
#ifdef CONFIG_NET_IPV6
#define ZEPHYR_ADDR		CONFIG_NET_CONFIG_MY_IPV6_ADDR
#define SERVER_ADDR		CONFIG_NET_CONFIG_PEER_IPV6_ADDR
#else
#define ZEPHYR_ADDR		CONFIG_NET_CONFIG_MY_IPV4_ADDR
#define SERVER_ADDR		CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#endif
#define SERVER_PORT             CONFIG_MQTT_PORT
#else
#ifdef CONFIG_NET_IPV6
#define ZEPHYR_ADDR		"2001:db8::1"
#define SERVER_ADDR		"2001:db8::2"
#else
#define ZEPHYR_ADDR		"192.168.1.101"
#define SERVER_ADDR		"5.196.95.208"
#endif
#endif

#if defined(CONFIG_SOCKS)
#define SOCKS5_PROXY_ADDR	SERVER_ADDR
#define SOCKS5_PROXY_PORT	1080
#endif

#ifdef CONFIG_MQTT_LIB_TLS
#ifdef CONFIG_MQTT_LIB_WEBSOCKET
#define SERVER_PORT		9001
#else
#define SERVER_PORT		8883
#endif /* CONFIG_MQTT_LIB_WEBSOCKET */
#else
#ifdef CONFIG_MQTT_LIB_WEBSOCKET
#define SERVER_PORT		9001
#else
#ifndef SERVER_PORT
#define SERVER_PORT		1883 //default TCP host value
#endif
#endif /* CONFIG_MQTT_LIB_WEBSOCKET */
#endif

#define APP_SLEEP_MSECS		500
#define APP_TX_RX_TIMEOUT       300
#define APP_NET_INIT_TIMEOUT    10000

#define APP_CONNECT_TRIES	15

#define APP_MAX_ITERATIONS	100

#define APP_MQTT_BUFFER_SIZE	256

#define MQTT_CLIENTID		"skyygate"


#ifdef CONFIG_MQTT_QOS_0_AT_MOST_ONCE
#define MQTT_QOS MQTT_QOS_0_AT_MOST_ONCE
#endif
#ifdef CONFIG_MQTT_QOS_1_AT_LEAST_ONCE
#define MQTT_QOS MQTT_QOS_1_AT_LEAST_ONCE
#endif
#ifdef CONFIG_MQTT_QOS_2_EXACTLY_ONCE
#define MQTT_QOS MQTT_QOS_2_EXACTLY_ONCE
#endif


#endif
