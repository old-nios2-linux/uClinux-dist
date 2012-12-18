#include <icc.h>
#include <protocol.h>
#include <debug.h>
#include <generated/autoconf.h>
#include <asm/portmux.h>

#define TEST_NUM 4

unsigned short bfin_peripheral_list[] = {P_SPI1_SCK, P_SPI1_MISO, P_SPI1_MOSI, 0};
resources_t bfin_peri_res = {
	.label = "bfin-spi1",
};

uint32_t resourceid_array[] = {
	RESMGR_ID(RESMGR_TYPE_GPIO, 40),
	RESMGR_ID(RESMGR_TYPE_SYS_IRQ, 52),
	RESMGR_ID(RESMGR_TYPE_DMA, 20),
	0,
};

void icc_task_init(int argc, char *argv[])
{
	int ret;
	int i = 0;

	bfin_peri_res.count = 3;
	bfin_peri_res.resources_array = (uint32_t)bfin_peripheral_list;

	COREB_DEBUG(1, "request resource id %s\n", bfin_peri_res.label);
	ret = sm_request_resource(EP_RESMGR_SERVICE, RESMGR_ID(RESMGR_TYPE_PERIPHERAL, 0), &bfin_peri_res);
	if (ret) {
		COREB_DEBUG(1, "request peri resource failed\n");
	}

	ret = sm_free_resource(EP_RESMGR_SERVICE, RESMGR_ID(RESMGR_TYPE_PERIPHERAL, 0), &bfin_peri_res);
	if (ret) {
		COREB_DEBUG(1, "free peri resource failed\n");
	}

	while (i < TEST_NUM) {

		if (resourceid_array[i] == 0)
			break;

		COREB_DEBUG(1, "request resource id %08x\n", resourceid_array[i]);
		ret = sm_request_resource(EP_RESMGR_SERVICE, resourceid_array[i], 0);
		if (ret) {
			COREB_DEBUG(1, "request resource failed\n");
		}
		i++;

	}

	i = 0;
	while (i < TEST_NUM) {

		if (resourceid_array[i] == 0)
			break;

		COREB_DEBUG(1, "request resource id %08x\n", resourceid_array[i]);
		ret = sm_free_resource(EP_RESMGR_SERVICE, resourceid_array[i], 0);
		if (ret) {
			COREB_DEBUG(1, "free resource failed\n");
		}
		i++;
	}


	COREB_DEBUG(1, "%s() end\n", __func__);
}

void icc_task_exit(void)
{
}

