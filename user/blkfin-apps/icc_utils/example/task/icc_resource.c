#include <icc.h>
#include <protocol.h>
#include <debug.h>
#include <generated/autoconf.h>
#include <asm/portmux.h>

#define TEST_NUM 4
enum {
	IRQ_TYPE_NONE           = 0x00000000,
	IRQ_TYPE_EDGE_RISING    = 0x00000001,
	IRQ_TYPE_EDGE_FALLING   = 0x00000002,
	IRQ_TYPE_EDGE_BOTH      = (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING),
	IRQ_TYPE_LEVEL_HIGH     = 0x00000004,
	IRQ_TYPE_LEVEL_LOW      = 0x00000008,
	IRQ_TYPE_LEVEL_MASK     = (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH),
	IRQ_TYPE_SENSE_MASK     = 0x0000000f,
	IRQ_TYPE_DEFAULT        = IRQ_TYPE_SENSE_MASK,

	IRQ_TYPE_PROBE          = 0x00000010,

	IRQ_LEVEL               = (1 <<  8),
};

unsigned short bfin_peripheral_list[] = {P_SPI1_SCK, P_SPI1_MISO, P_SPI1_MOSI, 0};
resources_t bfin_peri_res = {
	.label = "bfin-spi1",
};

uint32_t resourceid_array[] = {
	RESMGR_ID(RESMGR_TYPE_SYS_IRQ, IRQ_PINT4),
	RESMGR_ID(RESMGR_TYPE_GPIO, GPIO_PG14),
	0,
};


int bfin_gpio_output_value(int pin, int value)
{
	unsigned int bank;
	volatile struct gpio_port_t * gpiobank_reg;
	uint32_t gpiobit;

	bank = pin/16;

	gpiobit = 1 << (pin & 0xF);

	gpiobank_reg = (struct gpio_port_t *)(PORTA_FER + 0x80 * bank);

	gpiobank_reg->inen &= gpiobit;
	if (value)
		gpiobank_reg->data_set = gpiobit;
	else
		gpiobank_reg->data_clear = gpiobit;
	gpiobank_reg->dir_set = gpiobit;

	SSYNC();

	return 0;
}

int bfin_get_value(int pin)
{
	unsigned int bank;
	volatile struct gpio_port_t * gpiobank_reg;
	uint32_t gpiobit;

	bank = pin/16;

	gpiobit = 1 << (pin & 0xF);

	gpiobank_reg = (struct gpio_port_t *)(PORTA_FER + 0x80 * bank);

	return (1 & ((gpiobank_reg->data & gpiobit) >> (pin & 0xF)));

}

void handle_gpio_irq(uint32_t pin)
{
	COREB_DEBUG(1, "handle gpio irq %d\n", pin);
	bfin_gpio_output_value(GPIO_PG14, 1);
	delay(5);
	bfin_gpio_output_value(GPIO_PG14, 0);
}

static int bfin_handle_pint_irq(int irq)
{
	unsigned int bank;
	volatile struct bfin_pint_regs *pint;
	uint32_t pint_assign, pint_request, request, level_mask;
	uint32_t bit = 0;
	uint32_t pin = 0;

	bank = irq - IRQ_PINT0;
	pint = (struct bfin_pint_regs *)(PINT0_MASK_SET + 0x100 * bank);
	pint_assign = pint->assign;
	pint_request = request = pint->request;
	level_mask = pint->edge_set & request;

	pint->request = pint_request;
	pint->mask_clear = pint_request;

	while (request) {
		if (request & 1) {
			pin = 16 * bank + 16 * ((pint_assign >> (bit/8 * 8)) & 0xff) + (bit % 16);
			COREB_DEBUG(1, "pint irq %d %d\n", bank, bit);
			handle_gpio_irq(pin);
		}
		bit++;
		request >>= 1;
	}

	pint->mask_set = pint_request;
}

static int bfin_setup_pint_irq(int irq, int pin, unsigned long type)
{
	unsigned int bank;
	volatile struct bfin_pint_regs *pint;
	volatile struct gpio_port_t * gpiobank_reg;
	uint32_t pintbit;
	uint32_t gpiobit;

	bank = irq - IRQ_PINT0;

	pintbit = 1 << ((pin + 16 * (pint->assign & 0xf)) & 0x1F);
	gpiobit = 1 << (pin & 0x1F);

	pint = (struct bfin_pint_regs *)(PINT0_MASK_SET + 0x100 * bank);

	gpiobank_reg = (struct gpio_port_t *)(PORTA_FER + 0x80 * bank);

	gpiobank_reg->dir_clear = gpiobit;
	gpiobank_reg->inen |= gpiobit;


	if (type & (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING)) {
		pint->edge_set = pintbit;
	} else {
		pint->edge_clear = pintbit;
	}

	if ((type & (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_LEVEL_LOW))) {
		pint->invert_set = pintbit;
	} else {
		pint->invert_clear = pintbit;
	}

	pint->mask_set = pintbit;

	SSYNC();

	return 0;
}

void icc_task_init(int argc, char *argv[])
{
	int ret;
	int i = 0;

	bfin_peri_res.count = 3;
	bfin_peri_res.resources_array = (uint32_t)bfin_peripheral_list;

	COREB_DEBUG(1, "request resource id %s\n", bfin_peri_res.label);
	ret = sm_request_resource(0, RESMGR_ID(RESMGR_TYPE_PERIPHERAL, 0), &bfin_peri_res);
	if (ret) {
		COREB_DEBUG(1, "request peri resource failed\n");
	}

	while (i < TEST_NUM) {

		if (resourceid_array[i] == 0)
			break;

		COREB_DEBUG(1, "request resource id %08x\n", resourceid_array[i]);
		ret = sm_request_resource(0, resourceid_array[i], 0);
		if (ret) {
			COREB_DEBUG(1, "request resource failed\n");
		}

		i++;

	}

	icc_register_irq_handle(IRQ_PINT4, bfin_handle_pint_irq);
	bfin_setup_pint_irq(IRQ_PINT4, 12, IRQ_TYPE_EDGE_RISING);

	COREB_DEBUG(1, "%s() end\n", __func__);
}

void icc_task_exit(void)
{
	int i;
	sm_free_resource(0, RESMGR_ID(RESMGR_TYPE_PERIPHERAL, 0), &bfin_peri_res);

	while (i < TEST_NUM) {
		if (resourceid_array[i] == 0)
			break;
		sm_free_resource(0, resourceid_array[i], 0);
	}

	icc_unregister_irq_handle(IRQ_PINT4);
}

