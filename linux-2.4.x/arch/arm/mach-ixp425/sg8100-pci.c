/*
 * arch/arm/mach-ixp425/sg8100-pci.c 
 *
 * SecureComputing/SG8100 PCI initialization
 *
 * Copyright (C) 2002 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/config.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/delay.h>

#include <asm/mach/pci.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/arch/pci.h>


/* PCI controller pin mappings */
#define INTA_PIN	IXP425_GPIO_PIN_8

void __init sg8100_pci_init(void *sysdata)
{
	gpio_line_config(INTA_PIN, IXP425_GPIO_IN | IXP425_GPIO_ACTIVE_LOW);
	gpio_line_isr_clear(INTA_PIN);
	ixp425_pci_init(sysdata);
}

static int __init sg8100_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	if (slot == 16)
		return IRQ_SG8100_PCI_INTA;
	return -1;
}

void sg8100_cardbus_fixup(struct pci_controller *hose, int current_bus, int pci_devfn)
{
	unsigned int scr, mfr;
	unsigned short bcr;

	/* Leave the cardbus slots in the reset state for now */
	early_read_config_word(hose, current_bus, pci_devfn, 0x3e, &bcr);
	bcr |= 0x0040;	/* RESET the cardbus slot */
	bcr &= ~0x0080;	/* Enable PCI interrupt routing */
	early_write_config_word(hose, current_bus, pci_devfn, 0x3e, bcr);

	/* Enable MFUNC0 to be interrupt source for slot */
	early_read_config_dword(hose, current_bus, pci_devfn, 0x80, &scr);
	scr |= 0x28000000;
	early_write_config_dword(hose, current_bus, pci_devfn, 0x80, scr);
	early_read_config_dword(hose, current_bus, pci_devfn, 0x80, &scr);

	early_read_config_dword(hose, current_bus, pci_devfn, 0x8c, &mfr);
	mfr &= ~0x0000000f;
	mfr |= 0x00000002;
	early_write_config_dword(hose, current_bus, pci_devfn, 0x8c, mfr);
	early_read_config_dword(hose, current_bus, pci_devfn, 0x8c, &mfr);
}

struct hw_pci sg8100_pci __initdata = {
	init:		sg8100_pci_init,
	swizzle:	common_swizzle,
	map_irq:	sg8100_map_irq,
};

