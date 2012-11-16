#include <icc.h>
#include <protocol.h>
#include <debug.h>

#define TEST_NUM 4

uint32_t __icc_task_data resource_array[] = {
	RESMGR_ID(RESMGR_TYPE_PERIPHERAL, 2),
	RESMGR_ID(RESMGR_TYPE_GPIO, 40),
	RESMGR_ID(RESMGR_TYPE_SYS_IRQ, 52),
	RESMGR_ID(RESMGR_TYPE_DMA, 20),
	0,
};

void  __icc_task icc_task_init(int argc, char *argv[])
{
	int ret;
	int i = 0;

	while (i < TEST_NUM) {

		COREB_DEBUG(1, "request resource id %08x\n", resource_array[i]);
		ret = sm_request_resource(0, resource_array[i]);
		if (ret <= 0) {
			COREB_DEBUG(1, "request resource failed\n");
		}
		i++;

	}

	i = 0;
	while (i < TEST_NUM) {

		COREB_DEBUG(1, "request resource id %08x\n", resource_array[i]);
		ret = sm_free_resource(0, resource_array[i]);
		if (ret <= 0) {
			COREB_DEBUG(1, "request resource failed\n");
		}
		i++;
	}


	COREB_DEBUG(1, "%s() end\n", __func__);
}

void  __icc_task icc_task_exit(void)
{
}

