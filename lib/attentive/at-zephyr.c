/*
 * Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License, Version 2, as
 * published by Sam Hocevar. See the COPYING file for more details.
 */

#include <zephyr.h>
#include <at.h>

/* Zephyr files. */
#include <kernel.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <drivers/gpio.h>
#include <device.h>
#include <init.h>

/* Net files (probably not required). */
#include <net/net_if.h>
#include <net/net_offload.h>
#include <net/socket_offload.h>
#include <net/socket_offload_ops.h>

/* Modem files. */
#include "../drivers/modem/modem_context.h"
#include "../drivers/modem/modem_socket.h"
#include "../drivers/modem/modem_cmd_handler.h"
#include "../drivers/modem/modem_iface_uart.h"

/* Remove once you re-factor this out. */
#define AT_COMMAND_LENGTH 128

struct at_zephyr {
    struct at    at;
    int          timeout;                /**< Command timeout in seconds. */
    const char   *response;
	struct k_sem at_sem;                 /* Semaphore which is acquired to wait for a response from the system. */
    bool         waiting;                /**< Waiting for response callback to arrive. */
};

/* RX thread structures */
K_THREAD_STACK_DEFINE(modem_rx_stack, 8192);
struct k_thread modem_rx_thread;

/* Static Variables for Zephyr AT Layer. */
static struct at_zephyr               zephyr_at_device;
static struct modem_iface             *uart_at_interface;
static struct modem_iface_uart_data   *uart_iface_data;

/* RX thread */
static void modem_rx(void)
{
	size_t bytes_read;
	int    ret;

	char   read_buf[128];

	while (true) {
		bytes_read = 0;

		/* wait for incoming data */
		k_sem_take(&uart_iface_data->rx_sem, K_FOREVER);

		/* Read some data from the interface. */
		ret = uart_at_interface->read(uart_at_interface, read_buf, 128, &bytes_read);
		if (ret == 0) {
			/* mctx.cmd_handler.process(&mctx.cmd_handler, &mctx.iface);
			 * */
			at_parser_feed(zephyr_at_device.at.parser, read_buf, bytes_read);
		}

		/* give up time if we have a solid stream of data */
		k_yield();
	}
}

static void handle_response(const char *buf, size_t len, void *arg)
{
    struct at_zephyr *priv = (struct at_zephyr *) arg;

    /* The mutex is held by the reader thread; don't reacquire. */
    priv->response = buf;
    (void) len;
    priv->waiting = false;

    /* Indicate that we've had a response from the system. */
    k_sem_give(&priv->at_sem);
}

static void handle_urc(const char *buf, size_t len, void *arg)
{
    struct at *at = (struct at *) arg;

    /* Forward to caller's URC callback, if any. */
    if (at->cbs->handle_urc)
        at->cbs->handle_urc(buf, len, at->arg);
}

enum at_response_type scan_line(const char *line, size_t len, void *arg)
{
    struct at *at = (struct at *) arg;

    enum at_response_type type = AT_RESPONSE_UNKNOWN;
    if (at->command_scanner)
        type = at->command_scanner(line, len, at->arg);
    if (!type && at->cbs && at->cbs->scan_line)
        type = at->cbs->scan_line(line, len, at->arg);
    return type;
}


/* Providing callbacks to the AT generic parser. */
static const struct at_parser_callbacks parser_callbacks = {
    .handle_response        = handle_response,
    .handle_urc             = handle_urc,
    .scan_line              = scan_line,
};

/* We are already aware of the device to be used with the Modem. This function will simply return
 * references to this device. */
struct at *at_alloc_zephyr(void)
{
	struct at_zephyr *priv = &zephyr_at_device;

    /* allocate underlying parser */
    priv->at.parser = at_parser_alloc(&parser_callbacks, 256, (void *) priv);
    if (!priv->at.parser) {
        free(priv);
        return NULL;
    }

    /* Initialize the response semaphore. */
    k_sem_init(&priv->at_sem, 0, 1);

    /* Return the pointer to AT device. */
    return &zephyr_at_device.at;
}

int at_open(struct at *at)
{
	/* Save pointer to UART AT Interface. */
	uart_at_interface = (struct modem_iface *) at->arg;
	uart_iface_data   = (struct modem_iface_uart_data *) uart_at_interface->iface_data;

	/* start RX thread */
	k_thread_create(&modem_rx_thread, modem_rx_stack,
			K_THREAD_STACK_SIZEOF(modem_rx_stack),
			(k_thread_entry_t) modem_rx,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

    return (0);
}

int at_close(struct at *at)
{
	/* No requirement to implement this right now! */
    return 0;
}

void at_free(struct at *at)
{
	struct at_zephyr *priv = (struct at_zephyr *) at;

    /* free up resources */
    free(priv->at.parser);
    free(priv);
}

void at_set_callbacks(struct at *at, const struct at_callbacks *cbs, void *arg)
{
    at->cbs = cbs;
    at->arg = arg;
}

void at_set_command_scanner(struct at *at, at_line_scanner_t scanner)
{
    at->command_scanner = scanner;
}

void at_set_timeout(struct at *at, int timeout)
{
    struct at_zephyr *priv = (struct at_zephyr *) at;

    /* Update the timeout in the structure. */
    priv->timeout = timeout;
}

void at_expect_dataprompt(struct at *at)
{
    at_parser_expect_dataprompt(at->parser);
}

static const char *_at_command(struct at_zephyr *priv, const void *data, size_t size, bool append_ctrlz)
{
	int   ret;
	u8_t  ctrlz = 0x1A;

    /* Prepare parser. */
    at_parser_await_response(priv->at.parser);

    /* Send the command. */
    uart_at_interface->write(uart_at_interface, data, size);

    /* Write Ctrl+Z if required. */
    if (append_ctrlz == true)
    	uart_at_interface->write(uart_at_interface, &ctrlz, 1);

    /* Wait for the parser thread to collect a response. */
    priv->waiting = true;
    if (priv->timeout) {
    	ret = k_sem_take(&priv->at_sem, K_SECONDS(priv->timeout));
    } else {
    	ret = k_sem_take(&priv->at_sem, -1);
    }

    const char *result;
    if (priv->waiting) {
        /* Timed out waiting for a response. */
        at_parser_reset(priv->at.parser);
        errno = ETIMEDOUT;
        result = NULL;
    } else {
        /* Response arrived. */
        result = priv->response;
    }

    /* Reset per-command settings. */
    priv->at.command_scanner = NULL;

    /* Result -> Caller ! */
    return (result);
}

const char *at_command(struct at *at, const char *format, ...)
{
	struct at_zephyr *priv = (struct at_zephyr *) at;

    /* Build command string. */
    va_list ap;
    va_start(ap, format);
    char line[AT_COMMAND_LENGTH];
    int len = vsnprintf(line, sizeof(line)-1, format, ap);
    va_end(ap);

    /* Bail out if we run out of space. */
    if (len >= (int)(sizeof(line)-1)) {
        errno = ENOMEM;
        return NULL;
    }

    /* Append modem-style newline. */
    line[len++] = '\r';

    /* Send the command. */
    return _at_command(priv, line, len, false);
}

const char *at_command_raw(struct at *at, const void *data, size_t size)
{
    struct at_zephyr *priv = (struct at_zephyr *) at;

    /* Spit out the command on the wire. */
    return _at_command(priv, data, size, true);
}
