
/*
 * (C) Copyright 2016 Nexell
 * Sungwoo, Park <swpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>

#include <pwm-nexell.h>

#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_PWM_NX
enum gpio_group {
	gpio_a,	gpio_b, gpio_c, gpio_d, gpio_e,
};

struct pwm_device {
	int grp;
	int bit;
	int io_fn;
};

static struct pwm_device pwm_dev[] = {
	[0] = { .grp = gpio_d, .bit = 1,  .io_fn = 0 },
	[1] = { .grp = gpio_c, .bit = 13, .io_fn = 1 },
	[2] = { .grp = gpio_c, .bit = 14, .io_fn = 1 },
	[3] = { .grp = gpio_d, .bit = 0,  .io_fn = 0 },
};
#endif

static void board_backlight_disable(void)
{
#ifdef CONFIG_PWM_NX
	int gp_dual = pwm_dev[CONFIG_BACKLIGHT_CH].grp;
	int io_dual = pwm_dev[CONFIG_BACKLIGHT_CH].bit;
	int fn_dual = pwm_dev[CONFIG_BACKLIGHT_CH].io_fn;
#if SUPPORT_SECONDARY_DISPLAY
	int gp = pwm_dev[CONFIG_SBACKLIGHT_CH].grp;
	int io = pwm_dev[CONFIG_SBACKLIGHT_CH].bit;
	int fn = pwm_dev[CONFIG_SBACKLIGHT_CH].io_fn;
#endif
	/*****************************************************/
	/* Primary(RGBtoDualLVDS) : PWM0_LVDSLED*/
	nx_gpio_set_pad_function(gp_dual, io_dual, fn_dual);
	nx_gpio_set_output_value(gp_dual, io_dual, 0);
	nx_gpio_set_output_enable(gp_dual, io_dual, 1);

	/* Primary(RGBtoDualLVDS) : LVDSLEDEN */
	nx_gpio_set_pad_function(gpio_c, 14, 1);
	nx_gpio_set_output_value(gpio_c, 14, 0);
	nx_gpio_set_output_enable(gpio_c, 14, 1);

	/* Primary(RGBtoDualLVDS) : LVDSRST */
	nx_gpio_set_pad_function(gpio_e, 20, 0);
	nx_gpio_set_output_value(gpio_e, 20, 0);
	nx_gpio_set_output_enable(gpio_e, 20, 1);

	/* Primary(RGBtoDualLVDS) : 12VEN */
	nx_gpio_set_pad_function(gpio_e, 12, 0);
	nx_gpio_set_output_value(gpio_e, 12, 1);
	nx_gpio_set_output_enable(gpio_e, 12, 1);

#if SUPPORT_SECONDARY_DISPLAY
	/****************************************/
	/* Secondary(LVDS) : PWM3_SLED */
	nx_gpio_set_pad_function(gp, io, fn);
	nx_gpio_set_output_value(gp, io, 0);
	nx_gpio_set_output_enable(gp, io, 1);

	/* Secondary(LVDS) : SLED_EN */
	nx_gpio_set_pad_function(gpio_b, 21, 1);
	nx_gpio_set_output_value(gpio_b, 21, 0);
	nx_gpio_set_output_enable(gpio_b, 21, 1);

	/* Secondary(LVDS) : SLVDSSTB */
	nx_gpio_set_pad_function(gpio_c, 31, 0);
	nx_gpio_set_output_value(gpio_c, 31, 0);
	nx_gpio_set_output_enable(gpio_c, 31, 1);

	/* Secondary(LVDS) : SVLDSRST */
	nx_gpio_set_pad_function(gpio_c, 15, 0);
	nx_gpio_set_output_value(gpio_c, 15, 0);
	nx_gpio_set_output_enable(gpio_c, 15, 1);
#endif
#endif
}

static void board_backlight_enable(void)
{
#ifdef CONFIG_PWM_NX
	/* Primary(RGBtoDualLVDS) : PWM0_LVDSLED*/
	pwm_init(
		CONFIG_BACKLIGHT_CH,
		CONFIG_BACKLIGHT_DIV, CONFIG_BACKLIGHT_INV
		);
	pwm_config(
		CONFIG_BACKLIGHT_CH,
		TO_DUTY_NS(CONFIG_BACKLIGHT_DUTY, CONFIG_BACKLIGHT_HZ),
		TO_PERIOD_NS(CONFIG_BACKLIGHT_HZ)
		);
#if SUPPORT_SECONDARY_DISPLAY
	/* Secondary(LVDS) : PWM3_SLED */
	pwm_init(
		CONFIG_SBACKLIGHT_CH,
		CONFIG_SBACKLIGHT_DIV, CONFIG_SBACKLIGHT_INV
		);
	pwm_config(
		CONFIG_SBACKLIGHT_CH,
		TO_DUTY_NS(CONFIG_SBACKLIGHT_DUTY, CONFIG_SBACKLIGHT_HZ),
		TO_PERIOD_NS(CONFIG_SBACKLIGHT_HZ)
		);
#endif
#endif
}

int board_init(void)
{
	board_backlight_disable();
#ifdef CONFIG_MCU_DOWNLOAD
	nx_gpio_set_pad_function(gpio_b, 29, 3); /*UART4TXD */
	nx_gpio_set_output_enable(gpio_b, 29, 1);

	nx_gpio_set_pad_function(gpio_b, 28, 3); /* UART4RXD */
	nx_gpio_set_output_enable(gpio_b, 28, 1);
#endif

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

#ifdef CONFIG_SILENT_CONSOLE
	gd->flags &= ~GD_FLG_SILENT;
#endif

	/* RGB Alternate Function setting */
	nx_gpio_set_pad_function(gpio_a, 0, 1);
	nx_gpio_set_pad_function(gpio_a, 1, 1);
	nx_gpio_set_pad_function(gpio_a, 2, 1);
	nx_gpio_set_pad_function(gpio_a, 3, 1);
	nx_gpio_set_pad_function(gpio_a, 4, 1);
	nx_gpio_set_pad_function(gpio_a, 5, 1);
	nx_gpio_set_pad_function(gpio_a, 6, 1);
	nx_gpio_set_pad_function(gpio_a, 7, 1);
	nx_gpio_set_pad_function(gpio_a, 8, 1);
	nx_gpio_set_pad_function(gpio_a, 9, 1);
	nx_gpio_set_pad_function(gpio_a, 10, 1);
	nx_gpio_set_pad_function(gpio_a, 11, 1);
	nx_gpio_set_pad_function(gpio_a, 12, 1);
	nx_gpio_set_pad_function(gpio_a, 13, 1);
	nx_gpio_set_pad_function(gpio_a, 14, 1);
	nx_gpio_set_pad_function(gpio_a, 15, 1);
	nx_gpio_set_pad_function(gpio_a, 16, 1);
	nx_gpio_set_pad_function(gpio_a, 17, 1);
	nx_gpio_set_pad_function(gpio_a, 18, 1);
	nx_gpio_set_pad_function(gpio_a, 19, 1);
	nx_gpio_set_pad_function(gpio_a, 20, 1);
	nx_gpio_set_pad_function(gpio_a, 21, 1);
	nx_gpio_set_pad_function(gpio_a, 22, 1);
	nx_gpio_set_pad_function(gpio_a, 23, 1);
	nx_gpio_set_pad_function(gpio_a, 24, 1);
	nx_gpio_set_pad_function(gpio_a, 25, 1);
	nx_gpio_set_pad_function(gpio_a, 26, 1);
	nx_gpio_set_pad_function(gpio_a, 27, 1);

	/* Primary(RGBtoDualLVDS) : Ready */
	nx_gpio_set_output_value(gpio_e, 12, 1);	/* 12VEN */
	udelay(20);
	nx_gpio_set_output_value(gpio_e, 20, 1);	/* LVDSRST */
#if 0
	nx_gpio_set_output_value(gpio_e, 0, 1);		/* LVDSSTBY */
#endif
	nx_gpio_set_output_value(gpio_c, 14, 0);		/* LVDSLEDEN */
#if SUPPORT_SECONDARY_DISPLAY
	/* Secondary(LVDS) : Ready */
	nx_gpio_set_output_value(gpio_b, 21, 1);	/* SLED_EN */
	nx_gpio_set_output_value(gpio_c, 15, 1);		/* SVLDSRST */
	nx_gpio_set_output_value(gpio_c, 31, 1);		/* SLVDSSTB */
#endif
	board_backlight_enable();

#if 1	// AP_GPB26_CAM_PWEN
	nx_gpio_set_pad_function(gpio_b, 26, 1);
	nx_gpio_set_output_value(gpio_b, 26, 1);
	nx_gpio_set_output_enable(gpio_b, 26, 1);
#endif
#ifdef CONFIG_RECOVERY_BOOT
#define ALIVE_SCRATCH1_READ_REGISTER	(0xc00108b4)
#define ALIVE_SCRATCH1_RESET_REGISTER	(0xc00108ac)
#define RECOVERY_SIGNATURE				(0x52455343)    /* (ASCII) : R.E.S.C */
#ifndef QUICKBOOT
	printf("[con-svma] signature --> 0x%x\n", readl(ALIVE_SCRATCH1_READ_REGISTER));
#endif
	if (readl(ALIVE_SCRATCH1_READ_REGISTER) == RECOVERY_SIGNATURE) {
		printf("reboot recovery!!!!\n");
		writel(0xffffffff, ALIVE_SCRATCH1_RESET_REGISTER);
		setenv("bootcmd", "run recoveryboot");
	}
#endif

#ifdef CONFIG_SYS_BURNING
	setenv("bootcmd", "fastboot 0");
	run_command("run bootcmd", 0);
#endif

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
	{
	.name = "mmc",
	.storage = SPLASH_STORAGE_MMC,
	.flags = SPLASH_STORAGE_RAW,
	.offset = CONFIG_SPLASH_MMC_OFFSET,
	},
};

int splash_screen_prepare(void)
{
	int err = splash_source_load(splash_locations,
		sizeof(splash_locations)/sizeof(struct splash_location));
	if (!err) {
		char addr[64];

		sprintf(addr, "0x%lx", gd->fb_base);
		setenv("fb_addr", addr);
	}

	return err;
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