#include <zephyr.h>
#include <sys/printk.h>
#include <stdint.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <net/tls_credentials.h>

#include <net/socket.h>
#include <power/reboot.h>

#include "mqtt_publisher.h"
#include "uart_mqtt_bridge.h"
#include "board_lights.h"

#include "config.h"

/** "private" functions **/
int libs_init(void);
int modem_init(void);
int download_init(void);
int download_start(char *host, char *file);

LOG_MODULE_REGISTER(mesh_gateway, 3);
const char *build_str = "" __DATE__ " " __TIME__;

void main(void)
{
	k_sleep(1000);

	// init section
	printk("innblue > mqtt broker > mqtt-nrf91-nrf52-mesh\n");
	printk("innblue > %s [%d] > built: %s\n", __func__, __LINE__, build_str);

	board_lights.blink_mqtt(2);
	board_lights.blink_uart(2);
	board_lights.modem_connected(false);

	__ASSERT(0 == libs_init(), "Libraries cannot be initialized!");
	__ASSERT(0 == provision_certificates(), "Certificates cannot be provisioned!");

	// connect to LTE
	__ASSERT(0 == modem_init(), "Modem cannot be initialized!");
	board_lights.modem_connected(true);
	mqtt_publisher.initialize();

	printk("Setting up serial link to Bluetooth mesh...\n");
	uart_mqtt_bridge.start(CONFIG_BLE_UART_NAME, mqtt_publisher.publish);

	// set callback
	mqtt_publisher.set_subscribe_callback(uart_mqtt_bridge.write_to_uart);

	printk("connect to MQTT and start processing...\n");
	// once connected to MQTT keep processing input
	// or when cannot retry after a delay
	// this can be moved into internal thread of mqtt_publisher
	// in the future
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
