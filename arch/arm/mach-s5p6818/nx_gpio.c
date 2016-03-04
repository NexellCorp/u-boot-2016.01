/*
 * (C) Copyright 2016 Nexell
 * Youngbok, Park <hsjung@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

/*
 * FIXME : will be remove after support pinctrl
 */
#include <linux/types.h>
#include <asm/io.h>
#include "asm/arch/nx_gpio.h"
#define NUMBER_OF_GPIO_MODULE 5
u32 __g_nx_gpio_valid_bit[NUMBER_OF_GPIO_MODULE] = {
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};

static struct {
	struct nx_gpio_register_set *pregister;
} __g_module_variables[NUMBER_OF_GPIO_MODULE] = {
	{
		NULL,
	},
};

enum { nx_gpio_max_bit = 32 };

void nx_gpio_set_bit(u32 *value, u32 bit, int enable)
{
	register u32 newvalue;
	newvalue = *value;
	newvalue &= ~(1ul << bit);
	newvalue |= (u32)enable << bit;
	writel(newvalue, value);
}

int nx_gpio_get_bit(u32 value, u32 bit)
{
	return (int)((value >> bit) & (1ul));
}

void nx_gpio_set_bit2(u32 *value, u32 bit, u32 bit_value)
{
	register u32 newvalue = *value;
	newvalue = (u32)(newvalue & ~(3ul << (bit * 2)));
	newvalue = (u32)(newvalue | (bit_value << (bit * 2)));

	writel(newvalue, value);
}

u32 nx_gpio_get_bit2(u32 value, u32 bit)
{
	return (u32)((u32)(value >> (bit * 2)) & 3ul);
}

int nx_gpio_initialize(void)
{
	static int binit;
	u32 i;
	binit = 0;

	if (0 == binit) {
		for (i = 0; i < NUMBER_OF_GPIO_MODULE; i++)
			__g_module_variables[i].pregister = NULL;
		binit = true;
	}
	for (i = 0; i < NUMBER_OF_GPIO_MODULE; i++) {
		__g_nx_gpio_valid_bit[i] = 0xFFFFFFFF;
	};
	return true;
}

u32 nx_gpio_get_number_of_module(void)
{
	return NUMBER_OF_GPIO_MODULE;
}

u32 nx_gpio_get_size_of_register_set(void)
{
	return sizeof(struct nx_gpio_register_set);
}

void nx_gpio_set_base_address(u32 module_index, void *base_address)
{
	__g_module_variables[module_index].pregister =
		(struct nx_gpio_register_set *)base_address;
}

void *nx_gpio_get_base_address(u32 module_index)
{
	return (void *)__g_module_variables[module_index].pregister;
}

int nx_gpio_open_module(u32 module_index)
{
	register struct nx_gpio_register_set *pregister;
	pregister = __g_module_variables[module_index].pregister;
	writel(0xFFFFFFFF, &pregister->gpiox_slew_disable_default);
	writel(0xFFFFFFFF, &pregister->gpiox_drv1_disable_default);
	writel(0xFFFFFFFF, &pregister->gpiox_drv0_disable_default);
	writel(0xFFFFFFFF, &pregister->gpiox_pullsel_disable_default);
	writel(0xFFFFFFFF, &pregister->gpiox_pullenb_disable_default);
	return true;
}

int nx_gpio_close_module(u32 module_index) { return true; }

int nx_gpio_check_busy(u32 module_index) { return false; }

void nx_gpio_set_pad_function(u32 module_index, u32 bit_number,
					u32 padfunc) {
	register struct nx_gpio_register_set *pregister;
	pregister = __g_module_variables[module_index].pregister;
	nx_gpio_set_bit2(&pregister->gpioxaltfn[bit_number / 16],
			 bit_number % 16, padfunc);
}

void nx_gpio_set_pad_function32(u32 module_index, u32 msbvalue, u32 lsbvalue)
{
	register struct nx_gpio_register_set *pregister;
	pregister = __g_module_variables[module_index].pregister;
	writel(lsbvalue, &pregister->gpioxaltfn[0]);
	writel(msbvalue, &pregister->gpioxaltfn[1]);
}

int nx_gpio_get_pad_function(u32 module_index, u32 bit_number)
{
	register struct nx_gpio_register_set *pregister;
	pregister = __g_module_variables[module_index].pregister;
	return (int)nx_gpio_get_bit2(
		readl(&pregister->gpioxaltfn[bit_number / 16]),
		bit_number % 16);
}
