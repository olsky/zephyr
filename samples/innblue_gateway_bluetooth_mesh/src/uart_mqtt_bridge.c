#include "board_lights.h"
#include <drivers/uart.h>
#include <string.h>
#include <net/buf.h>
#include "uart_mqtt_bridge.h"
#include <zephyr.h>

#define MQTT_MSG_MAX_LEN (255)
#define MESH_MSG_MAX_LEN (255)

static K_FIFO_DEFINE(m_queue_to_mqtt);
static K_THREAD_STACK_DEFINE(uart_to_mqtt_thread_stack, 1024);
static struct k_thread uart_to_mqtt_thread_data;
NET_BUF_POOL_DEFINE(m_buf_pool_uart_to_mqtt, /*cnt*/ 40, /* size */ MESH_MSG_MAX_LEN + 1, 0, NULL);

static K_FIFO_DEFINE(m_queue_to_uart);
static K_THREAD_STACK_DEFINE(mqtt_to_uart_thread_stack, 1024);
static struct k_thread mqtt_to_uart_thread_data;
NET_BUF_POOL_DEFINE(m_buf_pool_mqtt_to_uart, /*cnt*/ 5, /* size */ MQTT_MSG_MAX_LEN + 1, 0, NULL);

static struct device *uart = NULL;
static bool (*m_write_to_mqtt)(char *msg);

static void uart_to_mqtt_thread(void *p1, void *p2, void *p3)
{
	while (1) {
		struct net_buf *buf;
		buf = net_buf_get(&m_queue_to_mqtt, K_FOREVER);
		if (m_write_to_mqtt && m_write_to_mqtt(buf->data))
			board_lights.blink_mqtt(3);
		net_buf_unref(buf);
		k_yield();
	}
}

static void mqtt_to_uart_thread(void *p1, void *p2, void *p3)
{
	while (1) {
		struct net_buf *buf;
		buf = net_buf_get(&m_queue_to_uart, K_FOREVER);
		printk("Writing to UART: %s\n", buf->data);
		while (*buf->data)
			uart_poll_out(uart, *buf->data++);
		net_buf_unref(buf);
		board_lights.blink_uart(3);
		k_yield();
	}
}

static void uart_isr(struct device *x)
{
	static u16_t rx_len = 0;
	static u8_t rx_buf[MESH_MSG_MAX_LEN + 1];
	u8_t rx_byte;
	struct net_buf *nbuf;

	while (uart_irq_update(uart) && uart_irq_is_pending(uart)) {
		if (!uart_irq_rx_ready(uart)) {
			if (uart_irq_tx_ready(uart))
				printk("transmit ready\n");
			else
				printk("spurious interrupt\n");
			break;
		}

		while (uart_fifo_read(uart, &rx_byte, 1)) {
			if (rx_byte != '\n') {
				rx_buf[rx_len] = rx_byte;
				// stop growing the message once max length is reached
				rx_len += (rx_len < MESH_MSG_MAX_LEN ? 1 : 0);
				continue;
			}

			if (0 == rx_len)
				continue;

			rx_buf[rx_len++]=0;

			board_lights.blink_uart(3);

			nbuf = net_buf_alloc(&m_buf_pool_uart_to_mqtt, K_NO_WAIT);
			if (nbuf) {
				net_buf_add_mem(nbuf,rx_buf, rx_len);
				net_buf_put(&m_queue_to_mqtt, nbuf);
			} else {
				printk("No more buffers to MQTT are available!\n");
			}

			nbuf = NULL;
			rx_len=0;
		}
	}
}

static void write_to_uart(const char *msg)
{
	const size_t msg_len = strlen(msg);
	struct net_buf *nbuf = net_buf_alloc(&m_buf_pool_mqtt_to_uart, K_NO_WAIT);
	if (nbuf) {
		net_buf_add_mem(nbuf,msg, msg_len <= MQTT_MSG_MAX_LEN ? msg_len : MQTT_MSG_MAX_LEN);
		net_buf_add_u8(nbuf,0);
		net_buf_put(&m_queue_to_uart, nbuf);
	} else {
		printk("No more buffers to UART are available!\n");
	}
}

static bool start (const char *uart_port,
	bool (*write_to_mqtt)(char *msg))
{
	if (!write_to_mqtt)
		return false;

	m_write_to_mqtt = write_to_mqtt;

	printk("Setting up serial.\n");
	uart = device_get_binding(uart_port);

	if (uart == NULL) {
		printk("Device not found: %s\n", uart_port);
		return false;
	}

	printk("uart device found, configuring...\n");
	uart_irq_rx_disable(uart);
	uart_irq_tx_disable(uart);

	// setting
	uart_irq_callback_set(uart, uart_isr);
	printk("\t - set uart callback\n");

	uart_irq_rx_enable(uart);
	printk("\t - enable RX\n");

	k_thread_create(&uart_to_mqtt_thread_data, uart_to_mqtt_thread_stack,
		K_THREAD_STACK_SIZEOF(uart_to_mqtt_thread_stack), uart_to_mqtt_thread,
		NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	k_thread_create(&mqtt_to_uart_thread_data, mqtt_to_uart_thread_stack,
		K_THREAD_STACK_SIZEOF(mqtt_to_uart_thread_stack), mqtt_to_uart_thread,
		NULL, NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);

	return true;
}

const uart_mqtt_bridge_t uart_mqtt_bridge = {
	.start = start,
	.write_to_uart = write_to_uart
};
