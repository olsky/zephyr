/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>
#include <drivers/gpio.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* Pin assignments for Innblue skyygate sim868 board. */
static const struct pin_config pinconf[] = {

#if DT_HAS_NODE_STATUS_OKAY(DT_NODELABEL(usart1))
	/* STM32_PUPDR_PULL_UP Pull ups required for interurpt mode config.*/
	{STM32_PIN_PB6, STM32L4X_PINMUX_FUNC_PB6_USART1_TX | STM32_PUPDR_PULL_UP},
	{STM32_PIN_PB7, STM32L4X_PINMUX_FUNC_PB7_USART1_RX | STM32_PUPDR_PULL_UP},
#endif	/* CONFIG_UART_1 */

#if DT_HAS_NODE_STATUS_OKAY(DT_NODELABEL(usart2))
	{STM32_PIN_PD5, STM32L4X_PINMUX_FUNC_PD5_USART2_TX },
	{STM32_PIN_PD6, STM32L4X_PINMUX_FUNC_PD6_USART2_RX },
#endif	/* CONFIG_UART_2 */

#if 0 /* Not required for now. */

#ifdef CONFIG_UART_4
	/* STM32_PUPDR_PULL_UP Pull ups required for interurpt mode config.*/
	{STM32_PIN_PA0, STM32L4X_PINMUX_FUNC_PA0_USART4_TX},
	{STM32_PIN_PA1, STM32L4X_PINMUX_FUNC_PA1_USART4_RX},
#endif	/* CONFIG_UART_4 */

#ifdef CONFIG_SPI_3
	/* SPI3 is used as SPI HCI for BLE. */
	{STM32_PIN_PC10, STM32L4X_PINMUX_FUNC_PC10_SPI3_SCK},
	{STM32_PIN_PC11, STM32L4X_PINMUX_FUNC_PC11_SPI3_MISO},
	{STM32_PIN_PC12, STM32L4X_PINMUX_FUNC_PC12_SPI3_MOSI},
#endif /* CONFIG_SPI_3 */

#ifdef CONFIG_I2C_2
	/* I2C2 is used for various sensors */
	{STM32_PIN_PB10, STM32L4X_PINMUX_FUNC_PB10_I2C2_SCL},
	{STM32_PIN_PB11, STM32L4X_PINMUX_FUNC_PB11_I2C2_SDA},
#endif /* CONFIG_I2C_2 */

#endif /* #if 0 */
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);
	
	/* Setup pins for STM32. */
	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

static int pwr_ctrl_init(struct device *dev)
{
	/* Now that the pins are setup, we need to power up the modem.
	   This requires us to configure both PC0 / PE6 as output and set
	   them high.

	   Note that eventually it might be good to put this in a different function. */

	/* Configure PC0 / PE6 as output. */
	struct device *power_pin_pc0 = device_get_binding(DT_ST_STM32_GPIO_48000800_LABEL);
	struct device *gsm_pin_pe6 = device_get_binding(DT_ST_STM32_GPIO_48001000_LABEL);
	struct device *gnss_pin = device_get_binding(DT_ST_STM32_GPIO_48001C00_LABEL);

	gpio_pin_configure(power_pin_pc0, 0, GPIO_OUTPUT);
	gpio_pin_configure(gsm_pin_pe6, 6, GPIO_OUTPUT);
	gpio_pin_configure(gnss_pin, 1, GPIO_OUTPUT);

	/* PC0 Low -> Wait -> High -> Wait. */
	printk("init_modem > sim8xx > power Off\n");
	gpio_pin_set(power_pin_pc0, 0, 0);
	k_sleep(K_SECONDS(1));
	/* PC6 High  */
	printk("init_modem > sim8xx > gsm ON\n");
	gpio_pin_set(gsm_pin_pe6, 6, 1);
	printk("init_modem > sim8xx > gnss ON\n");
	gpio_pin_set(gnss_pin, 1, 1);
	printk("init_modem > sim8xx > power ON\n");
	gpio_pin_set(power_pin_pc0, 0, 1);

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	     CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);

// after 45 which is pinmux prio 
DEVICE_INIT(pwr_ctrl_init, "", pwr_ctrl_init, NULL, NULL,
			POST_KERNEL, 46);
