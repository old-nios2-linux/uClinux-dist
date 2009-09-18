/*
 * CoreB side simple gpio function for BF561.
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/err.h>
#include <mach/defBF561.h>
#include <asm/blackfin.h>
#include <asm/gpio.h>
#include <mach/gpio.h>

struct gpio_port_t * const gpio_array[] = {
	(struct gpio_port_t *) FIO0_FLAG_D,
	(struct gpio_port_t *) FIO1_FLAG_D,
	(struct gpio_port_t *) FIO2_FLAG_D,
};

inline int check_gpio(unsigned gpio)
{
	if (gpio >= MAX_BLACKFIN_GPIOS)
		return -EINVAL;
	return 0;
}

static inline void __bfin_gpio_direction_input(unsigned gpio)
{
	gpio_array[gpio_bank(gpio)]->dir &= ~gpio_bit(gpio);
	gpio_array[gpio_bank(gpio)]->inen |= gpio_bit(gpio);
}

int bfin_gpio_direction_input(unsigned gpio)
{
	__bfin_gpio_direction_input(gpio);

	return 0;
}

void bfin_gpio_set_value(unsigned gpio, int arg)
{
	if (arg)
		gpio_array[gpio_bank(gpio)]->data_set = gpio_bit(gpio);
	else
		gpio_array[gpio_bank(gpio)]->data_clear = gpio_bit(gpio);
}

int bfin_gpio_direction_output(unsigned gpio, int value)
{
	gpio_array[gpio_bank(gpio)]->inen &= ~gpio_bit(gpio);
	bfin_gpio_set_value(gpio, value);
	gpio_array[gpio_bank(gpio)]->dir |= gpio_bit(gpio);

	return 0;
}
