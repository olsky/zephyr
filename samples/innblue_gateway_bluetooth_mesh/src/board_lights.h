#ifndef BOARD_LIGHTS_H
#define BOARD_LIGHTS_H
#include <zephyr/types.h>
#include <stdbool.h>

typedef struct {
	void (*blink_uart) (u16_t blinks);
	void (*blink_mqtt) (u16_t blinks);
	void (*modem_connected) (bool connected);
} board_lights_t;

extern const board_lights_t board_lights;

#endif
