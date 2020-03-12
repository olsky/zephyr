/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/gpio.h>

#define VDD_5V0_PWR_CTRL_GPIO_PIN  21   // ENABLE_5V0_BOOST  --> speed sensor

static int pwr_ctrl_init(struct device *dev)
{
    const struct pwr_ctrl_cfg *cfg = dev->config->config_info;
    struct device *gpio;
    printk("innblue > %s [%d] > bind device \"%s\" pin %d\n", __func__, __LINE__, DT_GPIO_P0_DEV_NAME, VDD_5V0_PWR_CTRL_GPIO_PIN);

    gpio = device_get_binding(DT_GPIO_P0_DEV_NAME);
    if (!gpio) {
        printk("innblue > %s [%d] > Could not bind device \"%s\"\n", __func__, __LINE__, DT_GPIO_P0_DEV_NAME);
        return -ENODEV;
    }

    gpio_pin_configure(gpio, VDD_5V0_PWR_CTRL_GPIO_PIN, GPIO_DIR_OUT);
    gpio_pin_write(gpio, VDD_5V0_PWR_CTRL_GPIO_PIN, 1);

    k_sleep(1); /* Wait for the rail to come up and stabilize */

    return 0;
}

DEVICE_INIT(vdd_5v0_pwr_ctrl_init, "", pwr_ctrl_init, NULL, NULL,
            POST_KERNEL, 70);

        

