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
#ifdef CONFIG_UART_1
	{STM32_PIN_PB6, STM32L4X_PINMUX_FUNC_PB6_USART1_TX | STM32_PUPDR_PULL_UP},
	{STM32_PIN_PB7, STM32L4X_PINMUX_FUNC_PB7_USART1_RX | STM32_PUPDR_PULL_UP},
#endif	/* CONFIG_UART_1 */
#ifdef CONFIG_UART_2
	{STM32_PIN_PD5, STM32L4X_PINMUX_FUNC_PD5_USART2_TX | STM32_PUPDR_PULL_UP},
	{STM32_PIN_PD6, STM32L4X_PINMUX_FUNC_PD6_USART2_RX | STM32_PUPDR_PULL_UP},
	{STM32_PIN_PD3, STM32L4X_PINMUX_FUNC_PD3_USART2_CTS},
	{STM32_PIN_PD4, STM32L4X_PINMUX_FUNC_PD4_USART2_RTS},
#endif	/* CONFIG_UART_2 */

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
};

/* Configures the pin as output and sets them high. */
static void config_pin(struct device *gpio, int pin)
{
	int err;

	/* Configure this pin as output. */
	err = gpio_pin_configure(gpio, pin, GPIO_OUTPUT_ACTIVE);
	if (err == 0) {
		/* Write "1" to this pin. */
		err = gpio_pin_set(gpio, pin, 1);
	}
}

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	/* Setup pins for STM32. */
	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	/* Now that the pins are setup, we need to power up the modem. 
	   This requires us to configure both PC0 / PE6 as output and set 
       them high. 
	   
	   Note that eventually it might be good to put this in a different function. */
	config_pin(device_get_binding(DT_INST_2_ST_STM32_GPIO_LABEL), 0);
	config_pin(device_get_binding(DT_INST_5_ST_STM32_GPIO_LABEL), 6);
	
	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
