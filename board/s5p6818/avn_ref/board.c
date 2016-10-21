/*
 * (C) Copyright 2016 Nexell
 * Hyejung Kwon  <cjscld15@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
#ifdef CONFIG_PWM_NX
#include <pwm.h>
#endif
#include <asm/io.h>

#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>

DECLARE_GLOBAL_DATA_PTR;

enum gpio_group {
	gpio_a,	gpio_b, gpio_c, gpio_d, gpio_e,
};

int board_init(void)
{
	/* set pwm0 output off: 1 */
	nx_gpio_set_pad_function(gpio_d,  1, 0);
	nx_gpio_set_output_value(gpio_d,  1, 1);
	nx_gpio_set_output_enable(gpio_d,  1, 1);

#ifdef CONFIG_SILENT_CONSOLE
	gd->flags |= GD_FLG_SILENT;
#endif

	return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
#ifdef CONFIG_SILENT_CONSOLE
	gd->flags &= ~GD_FLG_SILENT;
#endif

	/* set lcd enable */
	nx_gpio_set_pad_function(gpio_c, 11, 1);
	nx_gpio_set_pad_function(gpio_b, 25, 1);
	nx_gpio_set_pad_function(gpio_b, 27, 1);

	nx_gpio_set_output_value(gpio_c, 11, 1);
	nx_gpio_set_output_value(gpio_b, 25, 1);
	nx_gpio_set_output_value(gpio_b, 27, 1);

	nx_gpio_set_output_enable(gpio_c, 11, 1);
	nx_gpio_set_output_enable(gpio_b, 25, 1);
	nx_gpio_set_output_enable(gpio_b, 27, 1);

	return 0;
}
#endif

#ifdef CONFIG_SPLASH_SOURCE
#include <splash.h>
struct splash_location splash_locations[] = {
	{
	.name = "mmc_fs",
	.storage = SPLASH_STORAGE_MMC,
	.flags = SPLASH_STORAGE_FS,
	.devpart = "0:1",
	},
};

int splash_screen_prepare(void)
{
	return splash_source_load(splash_locations,
				ARRAY_SIZE(splash_locations));
}
#endif

/* u-boot dram initialize  */
int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
	return 0;
}

/* u-boot dram board specific */
void dram_init_banksize(void)
{
	/* set global data memory */
	gd->bd->bi_arch_number = machine_arch_type;
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x00000100;

	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size  = CONFIG_SYS_SDRAM_SIZE;
}
