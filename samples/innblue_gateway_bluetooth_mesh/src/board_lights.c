#include "board_lights.h"
#include <device.h>
#include <drivers/gpio.h>
#include <zephyr.h>

#define LED_PORT	   "GPIO_0" //DT_ALIAS_LED0_GPIOS_CONTROLLER

#ifdef CONFIG_BOARD_NRF9160_PCA10090NS
	#define LED_MODEM_CTRL     LED_PORT
	#define LED_RECONNECT_CTRL LED_PORT
	#define LED_MQTT_CTRL      LED_PORT
	#define LED_UART_CTRL      LED_PORT
	// LTE modem led
	#define LED_MODEM DT_ALIAS_LED0_GPIOS_PIN
	// reconnect
	#define LED_RECONNECT DT_ALIAS_LED1_GPIOS_PIN
	// mqtt activity
	#define LED_MQTT DT_ALIAS_LED2_GPIOS_PIN
	// uart activity
	#define LED_UART DT_ALIAS_LED3_GPIOS_PIN
#elif defined(CONFIG_BOARD_NRF9160_PCA20035NS)
	#define LED_MODEM_CTRL     LED_PORT
	#define LED_RECONNECT_CTRL LED_PORT
	#define LED_MQTT_CTRL      LED_PORT
	#define LED_UART_CTRL      LED_PORT
	// LTE modem led
	#define LED_MODEM DT_ALIAS_LED0_GPIOS_PIN
	// reconnect
	#define LED_RECONNECT DT_ALIAS_LED1_GPIOS_PIN
	// mqtt activity
	#define LED_MQTT DT_ALIAS_LED2_GPIOS_PIN
	// uart activity
	#define LED_UART DT_ALIAS_LED2_GPIOS_PIN
#elif defined(CONFIG_BOARD_DISCO_L475_IOT1)
	#define LED_MODEM_CTRL     LED_PORT
	#define LED_RECONNECT_CTRL LED_PORT
	#define LED_MQTT_CTRL      LED_PORT
	#define LED_UART_CTRL      LED_PORT
	// LTE modem led
	#define LED_MODEM DT_ALIAS_LED0_GPIOS_PIN
	// reconnect
	#define LED_RECONNECT DT_ALIAS_LED1_GPIOS_PIN
	// mqtt activity
	#define LED_MQTT DT_ALIAS_LED1_GPIOS_PIN
	// uart activity
	#define LED_UART DT_ALIAS_LED1_GPIOS_PIN
#elif defined(CONFIG_BOARD_SKYYGATE_SIM868)
	#define LED_MODEM_CTRL     DT_GPIO_LEDS_LED0_GPIOS_CONTROLLER
	#define LED_RECONNECT_CTRL DT_GPIO_LEDS_LED1_GPIOS_CONTROLLER
	#define LED_MQTT_CTRL      DT_GPIO_LEDS_LED2_GPIOS_CONTROLLER
	#define LED_UART_CTRL      DT_GPIO_LEDS_LED3_GPIOS_CONTROLLER
	// LTE modem led
	#define LED_MODEM     DT_ALIAS_LED0_GPIOS_PIN
	// reconnect
	#define LED_RECONNECT DT_ALIAS_LED1_GPIOS_PIN
	// mqtt activity
	#define LED_MQTT      DT_ALIAS_LED2_GPIOS_PIN
	// uart activity
	#define LED_UART      DT_ALIAS_LED3_GPIOS_PIN
#endif


#define BLINK_DURATION_MS (250)

static K_THREAD_STACK_DEFINE(thread_stack, 256);
static struct k_thread thread_data;

K_MUTEX_DEFINE(led_modem_mutex);

//static struct device *dev;
struct led {
	struct device *dev;
	u32_t pin;
	u32_t blinks_left;
};

static struct led led_uart = {
	.pin = LED_UART,
	.blinks_left = 0
};

static struct led led_mqtt = {
	.pin = LED_MQTT,
	.blinks_left = 0
};

static struct led led_modem = {
	.pin = LED_MODEM,
	.blinks_left = 0
};

static void blink_down(struct led *light) {
	if (!light->blinks_left)
		return;

	if (light->blinks_left % 2 == 0)
		gpio_pin_set(light->dev, light->pin, 1);
	else
		gpio_pin_set(light->dev, light->pin, 0);

	light->blinks_left--;
}

static void blink_forever(struct led *light, struct k_mutex *mutex) {
	if (!light->blinks_left)
		return;

	k_mutex_lock(mutex, K_FOREVER);
	if (!light->blinks_left)
		goto exit;

	if (light->blinks_left % 2) {
		gpio_pin_set(light->dev, light->pin, 1);
		light->blinks_left = 2;
	}
	else {
		gpio_pin_set(light->dev, light->pin, 0);
		light->blinks_left = 1;
	}

exit:
	k_mutex_unlock(mutex);
}

static void state_thread(void *p1, void *p2, void *p3)
{
	while(1) {
		blink_down(&led_mqtt);
		blink_down(&led_uart);
		blink_forever(&led_modem, &led_modem_mutex);
		k_sleep(BLINK_DURATION_MS);
	}
}

static void initialise() {
	static bool initialised = false;

	if (initialised)
		return;
	

	// set device
	led_uart.dev  = device_get_binding(LED_UART_CTRL);
	led_mqtt.dev  = device_get_binding(LED_MQTT_CTRL);
	led_modem.dev = device_get_binding(LED_MODEM_CTRL);


	/* Set LED pin as output */
	gpio_pin_configure(led_modem.dev, LED_MODEM, GPIO_DIR_OUT);
	//gpio_pin_configure(dev, LED_RECONNECT, GPIO_DIR_OUT);
	gpio_pin_configure(led_uart.dev, LED_UART, GPIO_DIR_OUT);
	gpio_pin_configure(led_mqtt.dev, LED_MQTT, GPIO_DIR_OUT);

	k_thread_create(&thread_data, thread_stack,
		K_THREAD_STACK_SIZEOF(thread_stack), state_thread,
		NULL, NULL, NULL, K_PRIO_COOP(9), 0, K_NO_WAIT);

	initialised = true;
}

static void blink_mqtt (u16_t blinks) {
	initialise();
	if (led_mqtt.blinks_left == 0)
		led_mqtt.blinks_left = blinks * 2;
}

static void blink_uart (u16_t blinks) {
	initialise();
	if (led_uart.blinks_left == 0)
		led_uart.blinks_left = blinks * 2;
}

static void modem_connected (bool connected)
{
	initialise();
	if (connected) {
		k_mutex_lock(&led_modem_mutex, K_FOREVER);
		led_modem.blinks_left = 0;
		k_mutex_unlock(&led_modem_mutex);
		gpio_pin_set(led_modem.dev, led_modem.pin, 1);
	} else {
		k_mutex_lock(&led_modem_mutex, K_FOREVER);
		led_modem.blinks_left = 1;
		k_mutex_unlock(&led_modem_mutex);
	}
}

const board_lights_t board_lights = {
	.blink_mqtt = blink_mqtt,
	.blink_uart = blink_uart,
	.modem_connected = modem_connected
};