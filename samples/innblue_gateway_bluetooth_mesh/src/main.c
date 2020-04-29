
#include <logging/log.h>
LOG_MODULE_REGISTER(mesh_gateway, 3);

#include <zephyr.h>
#include <sys/printk.h>
#include <stdint.h>
#include <logging/log_ctrl.h>
#include <net/tls_credentials.h>
#include <net/socket.h>
#include <power/reboot.h>
#include <drivers/gpio.h>

/* App header files. */
#include "mqtt_publisher.h"
#include "uart_mqtt_bridge.h"
#include "board_lights.h"
#include "config.h"

#define LED0		DT_ALIAS_LED0_GPIOS_PIN
#define LED1		DT_ALIAS_LED1_GPIOS_PIN
#define LED2		DT_ALIAS_LED2_GPIOS_PIN
#define LED3		DT_ALIAS_LED3_GPIOS_PIN
#define LED4		DT_ALIAS_LED4_GPIOS_PIN
#define LED5		DT_ALIAS_LED5_GPIOS_PIN
#define SLEEP_TIME	50

void ui_blink(const char * controller, int led_pin, int times)
{
	for (int i = 0; i < 2*times; i++)
	{
		gpio_pin_set(device_get_binding(controller), led_pin, i % 2);
		k_sleep(SLEEP_TIME);
	}
	gpio_pin_set(device_get_binding(controller), led_pin, 1);
}



static struct net_mgmt_event_callback mgmt_cb;

static void event_handler(struct net_mgmt_event_callback *cb,
			  u32_t mgmt_event, struct net_if *iface)
{
	printk("event_handler evt: %d\n", mgmt_event);
	
	ui_blink(DT_ALIAS_LED3_GPIOS_CONTROLLER, LED3, 4);


	if ((mgmt_event & (NET_EVENT_L4_CONNECTED
			   | NET_EVENT_L4_DISCONNECTED)) != mgmt_event) {
		return;
	}

	// "User LD6 blue";

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected");
		
		gpio_pin_set(device_get_binding(DT_ALIAS_LED5_GPIOS_CONTROLLER), LED5, 0);
		return;
	}

	if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		LOG_INF("Network disconnected");

		gpio_pin_set(device_get_binding(DT_ALIAS_LED5_GPIOS_CONTROLLER), LED5, 1);
		return;
	}
}

#define INT_MODEM_STATUS_PIN 0
#define GPIOS_FLAG 0

void int_modem_status_handler(struct device *dev, struct gpio_callback *cb,
		    u32_t pins)
{
	static u32_t cnt = 0;
	//ui_blink(DT_ALIAS_LED1_GPIOS_CONTROLLER, LED1, 4);
	printk("\n\nint_modem_status_handler at %" PRIu32 "\n\n\n", k_cycle_get_32());
	// led_2 green
	gpio_pin_set(device_get_binding(DT_ALIAS_LED1_GPIOS_CONTROLLER), LED1, cnt % 2);
	cnt++;
}
// ok .. yours ;)

static struct gpio_callback cb_gpio_modem_status;

void init_modem_int(void)
{
	struct device *dev_modem_status;
	int ret;

	//printk("init_modem_int > get gpio device...\n");
	dev_modem_status = device_get_binding(DT_INST_3_ST_STM32_GPIO_LABEL);
	if (dev_modem_status == NULL) {
		printk("init_modem_int > Error: didn't find %s device\n",
			DT_INST_3_ST_STM32_GPIO_LABEL);
		return;
	}

	//printk("init_modem_int > config gpio pin\n");
	ret = gpio_pin_configure(dev_modem_status, INT_MODEM_STATUS_PIN,
				 GPIOS_FLAG | GPIO_INPUT);
	if (ret != 0) {
		printk("init_modem_int > Error %d: failed to configure pin %d \n",
			ret, INT_MODEM_STATUS_PIN);
		return;
	}

	//printk("init_modem_int > config pin interrupt\n");
	ret = gpio_pin_interrupt_configure(dev_modem_status, INT_MODEM_STATUS_PIN,
					   GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("init_modem_int > Error %d: failed to configure interrupt on pin %d\n",
			ret, INT_MODEM_STATUS_PIN);
		return;
	}

	//printk("init_modem_int > init callback\n");
	gpio_init_callback(&cb_gpio_modem_status, int_modem_status_handler,
			   BIT(INT_MODEM_STATUS_PIN));
	//printk("init_modem_int > set callback\n");
	gpio_add_callback(dev_modem_status, &cb_gpio_modem_status);
}

void init_modem(void)
{

	gpio_pin_configure(device_get_binding(DT_INST_2_ST_STM32_GPIO_LABEL), 0, GPIO_OUTPUT);

	printk("init_modem > sim8xx > power Off\n");
	gpio_pin_set(device_get_binding(DT_INST_2_ST_STM32_GPIO_LABEL), 0, 0);
	k_sleep(500);

	printk("init_modem > sim8xx > power ON\n");
	gpio_pin_set(device_get_binding(DT_INST_2_ST_STM32_GPIO_LABEL), 0, 1);
	k_sleep(500);

	gpio_pin_configure(device_get_binding(DT_INST_5_ST_STM32_GPIO_LABEL), 6, GPIO_OUTPUT);

	//printk("init_modem > sim8xx > gsm Off\n");
	//gpio_pin_set(device_get_binding(DT_INST_5_ST_STM32_GPIO_LABEL), 6, 0);
	//k_sleep(500);
	printk("init_modem > sim8xx > gsm ON\n");
	gpio_pin_set(device_get_binding(DT_INST_5_ST_STM32_GPIO_LABEL), 6, 1);
	//k_sleep(500);

	gpio_pin_configure(device_get_binding(DT_INST_3_ST_STM32_GPIO_LABEL), 1, GPIO_OUTPUT);

	//printk("init_modem > sim8xx > gnss Off\n");
	//gpio_pin_set(device_get_binding(DT_INST_3_ST_STM32_GPIO_LABEL), 1, 0);
	//k_sleep(500);
	printk("init_modem > sim8xx > gnss ON\n");
	gpio_pin_set(device_get_binding(DT_INST_3_ST_STM32_GPIO_LABEL), 1, 1);
	//k_sleep(500);
}

/* System main thread. */
void main(void)
{
	const char *build_str = "" __DATE__ " " __TIME__;

	/* Modem Init. */
	init_modem_int();
	init_modem();

	struct device *uart_dev = device_get_binding(CONFIG_MODEM_GSM_UART_NAME);
	LOG_INF("%s: booted board '%s' APN '%s' UART '%s' device %p",
		__func__, CONFIG_BOARD, CONFIG_MODEM_GSM_APN,
		CONFIG_MODEM_GSM_UART_NAME, uart_dev);

	printk("main > net_mgmt_init_event_callback...\n");
	net_mgmt_init_event_callback(&mgmt_cb, event_handler,
				     NET_EVENT_L4_CONNECTED |
				     NET_EVENT_L4_DISCONNECTED);
	printk("main > net_mgmt_add_event_callback...\n");
	net_mgmt_add_event_callback(&mgmt_cb);


	printk("innblue > mqtt broker > mqtt-nrf91-nrf52-mesh\n");
	printk("innblue > %s [%d] > built: %s\n", __func__, __LINE__, build_str);

#if 0
	/* Blink LEDs. */
	board_lights.blink_mqtt(2);
	board_lights.blink_uart(2);
	board_lights.modem_connected(false);

	/* Modem is already connected via PPP. Lets init mqtt. */
	board_lights.modem_connected(true);
	mqtt_publisher.initialize();

	/* Set up serial link to BLE Mesh. */
	printk("Setting up serial link to Bluetooth mesh...\n");
	uart_mqtt_bridge.start(CONFIG_BLE_UART_NAME, mqtt_publisher.publish);
	mqtt_publisher.set_subscribe_callback(uart_mqtt_bridge.write_to_uart);

	printk("connect to MQTT and start processing...\n");
	while (1) {
		if (!mqtt_publisher.process_input())
			k_sleep(K_MSEC(APP_SLEEP_MSECS));
	}
#endif

	while (1) {
		k_sleep(K_MSEC(1000));
	}
}

/**@brief Fatal error handler */
void k_sys_fatal_error_handler(unsigned int reason,
					const z_arch_esf_t *esf)
{
	ARG_UNUSED(esf);
	LOG_PANIC();
	LOG_ERR("Fatal error, the system is being restarted. Bye for now!");
	sys_reboot(SYS_REBOOT_COLD);
	CODE_UNREACHABLE;
}
