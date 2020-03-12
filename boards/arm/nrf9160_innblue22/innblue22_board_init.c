/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/gpio.h>

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

static const struct pwr_ctrl_cfg vdd_5v0_pwr_ctrl_cfg = {
    .port = DT_GPIO_P0_DEV_NAME,
    .pin  = VDD_5V0_PWR_CTRL_GPIO_PIN,
};

DEVICE_INIT(vdd_5v0_pwr_ctrl_init, "", pwr_ctrl_init, NULL, &vdd_5v0_pwr_ctrl_cfg,
        POST_KERNEL, 70);

        

