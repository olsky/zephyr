
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
#include "net_mngr.h"

/* System main thread. */
void main(void)
{
	const char *build_str = "" __DATE__ " " __TIME__;
	printk("innblue > mqtt broker > mqtt-nrf91-nrf52-mesh\n");
	printk("innblue > %s [%d] > built: %s\n", __func__, __LINE__, build_str);

	/* Blink LEDs. */
	board_lights.blink_mqtt(2);
	board_lights.blink_uart(2);
	board_lights.modem_connected(false);

	int net_wait_seconds = 120;
	while (net_wait_seconds && !net_mngr.is_network_ready()) {
		k_sleep(K_SECONDS(1));
		net_wait_seconds--;
	}

	if (net_mngr.is_network_ready() &&
		mqtt_publisher.initialize(net_mngr.get_guid())) 
		board_lights.modem_connected(true);
	else
		net_mngr.restart();
	

	/* Set up serial link to BLE Mesh. */
	printk("Setting up serial link to Bluetooth mesh...\n");
	uart_mqtt_bridge.start(CONFIG_BLE_UART_NAME, mqtt_publisher.publish);
	mqtt_publisher.set_subscribe_callback(uart_mqtt_bridge.write_to_uart);

	printk("connect to MQTT and start processing...\n");

	while (1) {
		if (!mqtt_publisher.process_input())
			net_mngr.restart();

		k_sleep(K_SECONDS(1));
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
