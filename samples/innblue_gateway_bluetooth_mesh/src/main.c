
#include <logging/log.h>
LOG_MODULE_REGISTER(mesh_gateway, 3);

#include <zephyr.h>
#include <sys/printk.h>
#include <stdint.h>
#include <logging/log_ctrl.h>
#include <net/tls_credentials.h>
#include <net/socket.h>
#include <power/reboot.h>

/* App header files. */
#include "mqtt_publisher.h"
#include "uart_mqtt_bridge.h"
#include "board_lights.h"
#include "config.h"

/* System main thread. */
void main(void)
{
	const char *build_str = "" __DATE__ " " __TIME__;

	/* TODO: Lets try to figure out a way to get rid of this delay. */
	k_sleep(1000);
	printk("innblue > mqtt broker > mqtt-nrf91-nrf52-mesh\n");
	printk("innblue > %s [%d] > built: %s\n", __func__, __LINE__, build_str);

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
