/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for ublox c030-r412 board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_1
	{STM32_PIN_PA9,  STM32F4_PINMUX_FUNC_PA9_USART1_TX},
	{STM32_PIN_PA10, STM32F4_PINMUX_FUNC_PA10_USART1_RX},
	{STM32_PIN_PA11, STM32F4_PINMUX_FUNC_PA11_USART1_CTS},
	{STM32_PIN_PA12, STM32F4_PINMUX_FUNC_PA12_USART1_RTS},
#endif /* CONFIG_UART_1 */
#ifdef CONFIG_UART_2
	{STM32_PIN_PD5, STM32F4_PINMUX_FUNC_PD5_USART2_TX},
	{STM32_PIN_PD6, STM32F4_PINMUX_FUNC_PD6_USART2_RX},
#endif /* CONFIG_UART_2 */
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	/* Setup pins for ublox board. */
	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
