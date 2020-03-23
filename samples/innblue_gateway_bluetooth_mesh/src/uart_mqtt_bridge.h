#ifndef UART_MQTT_BRIDGE_H
#define UART_MQTT_BRIDGE_H
#include <stdbool.h>

typedef	struct {
	bool (*start)(const char *uart_port,
		bool (*write_to_mqtt)(char *msg));

	void (*write_to_uart)(const char *msg);
} uart_mqtt_bridge_t;

extern const uart_mqtt_bridge_t uart_mqtt_bridge;

#endif
