#include <assert.h>

#include <device.h>
#include "net_mngr.h"
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <power/reboot.h>

#if defined(CONFIG_MODEM_GSM_PPP)
#include <net/ppp.h>
#include <../drivers/modem/modem_context.h>
#elif defined(CONFIG_MODEM_SIM800)
#include <../drivers/modem/simcomm-sim80x.h>
#endif


LOG_MODULE_REGISTER(net_mngr, 3);

#if defined(CONFIG_MODEM_GSM_PPP)
static const char* get_guid ()
{

	BUILD_ASSERT(CONFIG_MODEM_SHELL, 
		"CONFIG_MODEM_SHELL=y required to obtain EMEI");  

	struct modem_context *ctx = modem_context_from_iface_dev(
		device_get_binding(CONFIG_MODEM_GSM_UART_NAME));

	__ASSERT(ctx, "Cannot obtain modem context!");
	__ASSERT(*ctx->data_imei, "EMEI is empty!");

	return ctx->data_imei;
}

static bool is_network_ready ()
{
	BUILD_ASSERT(CONFIG_NET_L2_PPP && CONFIG_NET_SHELL,
		"CONFIG_NET_L2_PPP && CONFIG_NET_SHELL are required.");

	static struct ppp_context *ppp_ctx = NULL;
	if (!ppp_ctx)
		ppp_ctx = net_ppp_context_get(0);

	return ppp_ctx && ppp_ctx->is_network_up;
}

static void restart ()
{
	LOG_PANIC();
	LOG_ERR("Restarting the device to restart networking stack.");
	sys_reboot(SYS_REBOOT_COLD);
	CODE_UNREACHABLE;
}
#elif defined(CONFIG_MODEM_SIM800)
static const char* get_guid()
{
	struct device *modem = device_get_binding(CONFIG_MODEM_SIM80X_NAME);
	__ASSERT(modem, "Cannot find modem!");
	
	struct modem_data *mdm_data = (struct modem_data *) modem->driver_data;
	__ASSERT(modem_data.mdm_imei, "EMEI is empty!");
	return mdm_data->mdm_imei;		
}

static bool is_network_ready ()
{
	return true;
}


static void restart ()
{
	LOG_ERR("Restarting network.");
}
#endif

const net_mngr_t net_mngr = {
	.get_guid = get_guid,
	.is_network_ready = is_network_ready,
	.restart = restart
};
