/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_lptmr

#include <drivers/counter.h>
#include <fsl_lptmr.h>

struct mcux_lptmr_config {
	struct counter_config_info info;
	LPTMR_Type *base;
	lptmr_prescaler_clock_select_t clk_source;
	lptmr_prescaler_glitch_value_t prescaler_glitch;
	bool bypass_prescaler_glitch;
	lptmr_timer_mode_t mode;
	lptmr_pin_select_t pin;
	lptmr_pin_polarity_t polarity;
	void (*irq_config_func)(struct device *dev);
};

struct mcux_lptmr_data {
	counter_top_callback_t top_callback;
	void *top_user_data;
};

static int mcux_lptmr_start(struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config->config_info;

	LPTMR_EnableInterrupts(config->base,
			       kLPTMR_TimerInterruptEnable);
	LPTMR_StartTimer(config->base);

	return 0;
}

static int mcux_lptmr_stop(struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config->config_info;

	LPTMR_DisableInterrupts(config->base,
				kLPTMR_TimerInterruptEnable);
	LPTMR_StopTimer(config->base);

	return 0;
}

static int mcux_lptmr_get_value(struct device *dev, u32_t *ticks)
{
	const struct mcux_lptmr_config *config = dev->config->config_info;

	*ticks = LPTMR_GetCurrentTimerCount(config->base);

	return 0;
}

static int mcux_lptmr_set_top_value(struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct mcux_lptmr_config *config = dev->config->config_info;
	struct mcux_lptmr_data *data = dev->driver_data;

	if (cfg->ticks == 0) {
		return -EINVAL;
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (config->base->CSR & LPTMR_CSR_TEN_MASK) {
		/* Timer already enabled, check flags before resetting */
		if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
			return -ENOTSUP;
		}
		LPTMR_StopTimer(config->base);
		LPTMR_SetTimerPeriod(config->base, cfg->ticks);
		LPTMR_StartTimer(config->base);
	} else {
		LPTMR_SetTimerPeriod(config->base, cfg->ticks);
	}

	return 0;
}

static u32_t mcux_lptmr_get_pending_int(struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config->config_info;
	u32_t mask = LPTMR_CSR_TCF_MASK | LPTMR_CSR_TIE_MASK;
	u32_t flags;

	flags = LPTMR_GetStatusFlags(config->base);

	return ((flags & mask) == mask);
}

static u32_t mcux_lptmr_get_top_value(struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config->config_info;

	return (config->base->CMR & LPTMR_CMR_COMPARE_MASK) + 1U;
}

static u32_t mcux_lptmr_get_max_relative_alarm(struct device *dev)
{
	ARG_UNUSED(dev);

	/* Alarms not supported */
	return 0;
}

static void mcux_lptmr_isr(void *arg)
{
	struct device *dev = arg;
	const struct mcux_lptmr_config *config = dev->config->config_info;
	struct mcux_lptmr_data *data = dev->driver_data;
	u32_t flags;

	flags = LPTMR_GetStatusFlags(config->base);
	LPTMR_ClearStatusFlags(config->base, flags);

	if (data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
}

static int mcux_lptmr_init(struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config->config_info;
	lptmr_config_t lptmr_config;

	LPTMR_GetDefaultConfig(&lptmr_config);
	lptmr_config.timerMode = config->mode;
	lptmr_config.enableFreeRunning = false;
	lptmr_config.prescalerClockSource = config->clk_source;
	lptmr_config.bypassPrescaler = config->bypass_prescaler_glitch;
	lptmr_config.value = config->prescaler_glitch;

	if (config->mode == kLPTMR_TimerModePulseCounter) {
		lptmr_config.pinSelect = config->pin;
		lptmr_config.pinPolarity = config->polarity;
	}

	LPTMR_Init(config->base, &lptmr_config);

	config->irq_config_func(dev);

	return 0;
}

static const struct counter_driver_api mcux_lptmr_driver_api = {
	.start = mcux_lptmr_start,
	.stop = mcux_lptmr_stop,
	.get_value = mcux_lptmr_get_value,
	.set_top_value = mcux_lptmr_set_top_value,
	.get_pending_int = mcux_lptmr_get_pending_int,
	.get_top_value = mcux_lptmr_get_top_value,
	.get_max_relative_alarm = mcux_lptmr_get_max_relative_alarm,
};

#define TO_LPTMR_CLK_SEL(val) _DO_CONCAT(kLPTMR_PrescalerClock_, val)
#define TO_LPTMR_PIN_SEL(val) _DO_CONCAT(kLPTMR_PinSelectInput_, val)

/* Prescaler mapping */
#define LPTMR_PRESCALER_2     kLPTMR_Prescale_Glitch_0
#define LPTMR_PRESCALER_4     kLPTMR_Prescale_Glitch_1
#define LPTMR_PRESCALER_8     kLPTMR_Prescale_Glitch_2
#define LPTMR_PRESCALER_16    kLPTMR_Prescale_Glitch_3
#define LPTMR_PRESCALER_32    kLPTMR_Prescale_Glitch_4
#define LPTMR_PRESCALER_64    kLPTMR_Prescale_Glitch_5
#define LPTMR_PRESCALER_128   kLPTMR_Prescale_Glitch_6
#define LPTMR_PRESCALER_256   kLPTMR_Prescale_Glitch_7
#define LPTMR_PRESCALER_512   kLPTMR_Prescale_Glitch_8
#define LPTMR_PRESCALER_1024  kLPTMR_Prescale_Glitch_9
#define LPTMR_PRESCALER_2048  kLPTMR_Prescale_Glitch_10
#define LPTMR_PRESCALER_4096  kLPTMR_Prescale_Glitch_11
#define LPTMR_PRESCALER_8192  kLPTMR_Prescale_Glitch_12
#define LPTMR_PRESCALER_16384 kLPTMR_Prescale_Glitch_13
#define LPTMR_PRESCALER_32768 kLPTMR_Prescale_Glitch_14
#define LPTMR_PRESCALER_65536 kLPTMR_Prescale_Glitch_15
#define TO_LPTMR_PRESCALER(val) _DO_CONCAT(LPTMR_PRESCALER_, val)

/* Glitch filter mapping */
#define LPTMR_GLITCH_2     kLPTMR_Prescale_Glitch_1
#define LPTMR_GLITCH_4     kLPTMR_Prescale_Glitch_2
#define LPTMR_GLITCH_8     kLPTMR_Prescale_Glitch_3
#define LPTMR_GLITCH_16    kLPTMR_Prescale_Glitch_4
#define LPTMR_GLITCH_32    kLPTMR_Prescale_Glitch_5
#define LPTMR_GLITCH_64    kLPTMR_Prescale_Glitch_6
#define LPTMR_GLITCH_128   kLPTMR_Prescale_Glitch_7
#define LPTMR_GLITCH_256   kLPTMR_Prescale_Glitch_8
#define LPTMR_GLITCH_512   kLPTMR_Prescale_Glitch_9
#define LPTMR_GLITCH_1024  kLPTMR_Prescale_Glitch_10
#define LPTMR_GLITCH_2048  kLPTMR_Prescale_Glitch_11
#define LPTMR_GLITCH_4096  kLPTMR_Prescale_Glitch_12
#define LPTMR_GLITCH_8192  kLPTMR_Prescale_Glitch_13
#define LPTMR_GLITCH_16384 kLPTMR_Prescale_Glitch_14
#define LPTMR_GLITCH_32768 kLPTMR_Prescale_Glitch_15
#define TO_LPTMR_GLITCH(val) _DO_CONCAT(LPTMR_GLITCH_, val)

#if DT_HAS_DRV_INST(0)
static struct mcux_lptmr_data mcux_lptmr_data_0;

static void mcux_lptmr_irq_config_0(struct device *dev);

static struct mcux_lptmr_config mcux_lptmr_config_0 = {
	.info = {
		.max_top_value = UINT16_MAX,
		.freq = DT_INST_PROP(0, clock_frequency) /
			DT_INST_PROP(0, prescaler),
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,
		.channels = 0,
	},
	.base = (LPTMR_Type *)DT_INST_REG_ADDR(0),
	.clk_source = TO_LPTMR_CLK_SEL(DT_INST_PROP(0, clk_source)),
#if DT_INST_NODE_HAS_PROP(0, input_pin)
#if DT_INST_PROP(0, prescaler) == 1
	.bypass_prescaler_glitch = true,
#else
	.prescaler_glitch = TO_LPTMR_GLITCH(DT_INST_PROP(0, prescaler)),
#endif
	.mode = kLPTMR_TimerModePulseCounter,
	.pin = TO_LPTMR_PIN_SEL(DT_INST_PROP(0, input_pin)),
	.polarity = DT_INST_PROP(0, active_low),
#else /* !DT_INST_NODE_HAS_PROP(0, input_pin) */
	.mode = kLPTMR_TimerModeTimeCounter,
#if DT_INST_PROP(0, prescaler) == 1
	.bypass_prescaler_glitch = true,
#else
	.prescaler_glitch = TO_LPTMR_PRESCALER(DT_INST_PROP(0, prescaler)),
#endif
#endif /* !DT_INST_NODE_HAS_PROP(0, input_pin) */
	.irq_config_func = mcux_lptmr_irq_config_0,
};

DEVICE_AND_API_INIT(mcux_lptmr_0, DT_INST_LABEL(0),
		    &mcux_lptmr_init, &mcux_lptmr_data_0,
		    &mcux_lptmr_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_lptmr_driver_api);

static void mcux_lptmr_irq_config_0(struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    mcux_lptmr_isr, DEVICE_GET(mcux_lptmr_0), 0);
	irq_enable(DT_INST_IRQN(0));
}
#endif /* DT_HAS_DRV_INST(0) */
