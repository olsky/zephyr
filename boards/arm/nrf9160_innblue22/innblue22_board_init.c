/*
 * 2018, 2019, 2020 by innblue
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <init.h>
#include <drivers/gpio.h>

// always on: #define VDD_3V3_PWR_CTRL_GPIO_PIN  12   // ENABLE_3V3_SENSOR --> i2c sensors
#define VDD_5V0_PWR_CTRL_GPIO_PIN  21   // ENABLE_5V0_BOOST  --> speed sensor

struct pwr_ctrl_cfg {
    const char *port;
    u32_t pin;
};

static int pwr_ctrl_init(struct device *dev)
{
    const struct pwr_ctrl_cfg *cfg = dev->config->config_info;
    struct device *gpio;
    printk("innblue > %s [%d] > bind device \"%s\" pin %d\n", __func__, __LINE__, cfg->port, cfg->pin);

    gpio = device_get_binding(cfg->port);
    if (!gpio) {
        printk("innblue > %s [%d] > Could not bind device \"%s\"\n", __func__, __LINE__, cfg->port);
        return -ENODEV;
    }

    gpio_pin_configure(gpio, cfg->pin, GPIO_DIR_OUT);
    gpio_pin_write(gpio, cfg->pin, 1);

    k_sleep(1); /* Wait for the rail to come up and stabilize */

    return 0;
}

/*
 * The a sensor is connected to the Sensor_VDD power rail, which is downstream
 * from the VDD power rail. Both of these power rails need to be enabled before
 * the sensor driver init can be performed. The VDD rail also has to be powered up
 * before the Sensor_VDD rail. These checks are to enforce the power up sequence
 * constraints.
 */

/* ??? 
#if CONFIG_BOARD_VDD_PWR_CTRL_INIT_PRIORITY <= CONFIG_GPIO_NRF_INIT_PRIORITY
#error GPIO_NRF_INIT_PRIORITY must be lower than \
    BOARD_VDD_PWR_CTRL_INIT_PRIORITY
#endif
*/


/*
static const struct pwr_ctrl_cfg vdd_3v3_pwr_ctrl_cfg = {
    .port = DT_GPIO_P0_DEV_NAME,
    .pin  = VDD_3V3_PWR_CTRL_GPIO_PIN,
};

DEVICE_INIT(vdd_3v3_pwr_ctrl_init, "", pwr_ctrl_init, NULL, &vdd_3v3_pwr_ctrl_cfg,
        POST_KERNEL, 70);
*/
        
        
static const struct pwr_ctrl_cfg vdd_5v0_pwr_ctrl_cfg = {
    .port = DT_GPIO_P0_DEV_NAME,
    .pin  = VDD_5V0_PWR_CTRL_GPIO_PIN,
};

DEVICE_INIT(vdd_5v0_pwr_ctrl_init, "", pwr_ctrl_init, NULL, &vdd_5v0_pwr_ctrl_cfg,
        POST_KERNEL, 70);

        

