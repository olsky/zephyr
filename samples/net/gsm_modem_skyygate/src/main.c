/*
 * Main here -->

 * 
 */

#include <zephyr.h>
#include <device.h>
#include <inttypes.h>

#include <drivers/gpio.h>

#include <sys/util.h>
#include <sys/printk.h>

#include <drivers/uart.h>

#include <net/net_mgmt.h>
#include <net/net_event.h>
#include <net/net_conn_mgr.h>


#include <logging/log.h>
LOG_MODULE_REGISTER(sample_gsm_ppp, LOG_LEVEL_DBG);


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




void init_ui(void)
{
	u32_t cnt = 0;
	gpio_pin_configure(device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER), LED0, GPIO_OUTPUT);
	gpio_pin_set(device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER), LED0, 1);
	gpio_pin_configure(device_get_binding(DT_ALIAS_LED1_GPIOS_CONTROLLER), LED1, GPIO_OUTPUT);
	gpio_pin_set(device_get_binding(DT_ALIAS_LED1_GPIOS_CONTROLLER), LED1, 1);
	gpio_pin_configure(device_get_binding(DT_ALIAS_LED2_GPIOS_CONTROLLER), LED2, GPIO_OUTPUT);
	gpio_pin_set(device_get_binding(DT_ALIAS_LED2_GPIOS_CONTROLLER), LED2, 1);

	gpio_pin_configure(device_get_binding(DT_ALIAS_LED3_GPIOS_CONTROLLER), LED3, GPIO_OUTPUT);
	gpio_pin_configure(device_get_binding(DT_ALIAS_LED4_GPIOS_CONTROLLER), LED4, GPIO_OUTPUT);
	gpio_pin_configure(device_get_binding(DT_ALIAS_LED5_GPIOS_CONTROLLER), LED5, GPIO_OUTPUT);


	gpio_pin_set(device_get_binding(DT_ALIAS_LED3_GPIOS_CONTROLLER), LED3, 1);
	gpio_pin_set(device_get_binding(DT_ALIAS_LED4_GPIOS_CONTROLLER), LED4, 1);
	gpio_pin_set(device_get_binding(DT_ALIAS_LED5_GPIOS_CONTROLLER), LED5, 1);

	
	// oops.. always on if "odd"
	for (int i = 0; i < 4; ++i)
	{

		gpio_pin_set(device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER), LED0, cnt % 2);
		k_sleep(SLEEP_TIME);
		gpio_pin_set(device_get_binding(DT_ALIAS_LED1_GPIOS_CONTROLLER), LED1, cnt % 2);
		k_sleep(SLEEP_TIME);
		gpio_pin_set(device_get_binding(DT_ALIAS_LED2_GPIOS_CONTROLLER), LED2, cnt % 2);
		k_sleep(SLEEP_TIME);

		gpio_pin_set(device_get_binding(DT_ALIAS_LED3_GPIOS_CONTROLLER), LED3, cnt % 2);
		k_sleep(SLEEP_TIME);
		gpio_pin_set(device_get_binding(DT_ALIAS_LED4_GPIOS_CONTROLLER), LED4, cnt % 2);
		k_sleep(SLEEP_TIME);
		gpio_pin_set(device_get_binding(DT_ALIAS_LED5_GPIOS_CONTROLLER), LED5, cnt % 2);
		k_sleep(SLEEP_TIME);

		cnt++;
	}
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

	// try?


	//printk("pwr_ctrl_init > sim8xx > GNSS  ON\n");
	// PH1 is GPS ON output
	// config_pin_out_set(device_get_binding(DT_INST_3_ST_STM32_GPIO_LABEL), 1, 1);

	// there is an INPUT from sim800 module --> PH0 NET_STATUS 
	//printk("pwr_ctrl_init > sim8xx > config status input\n");
	//gpio_pin_configure(device_get_binding(DT_INST_3_ST_STM32_GPIO_LABEL), 0, GPIO_INPUT);

}


int main(void)
{
	printk("\n\n\n\n");
	printk("main > this is gsm_modem modified app...\n");

	init_ui();

	init_modem_int();

	init_modem();

	//k_sleep(K_MSEC(10000));

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


 while(1){k_sleep(K_MSEC(30000));		printk("main > wating 30s...\n\n\n");}


	return 0;
}
