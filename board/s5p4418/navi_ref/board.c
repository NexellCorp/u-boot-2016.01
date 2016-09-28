/*
 * (C) Copyright 2016 Nexell
 * Hyunseok, Jung <hsjung@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>

#include <asm/arch/nexell.h>
#include <asm/arch/clk.h>
#include <asm/arch/reset.h>
#include <asm/arch/nx_gpio.h>

#include <usb/dwc2_udc.h>

DECLARE_GLOBAL_DATA_PTR;

#if !defined CONFIG_OF_CONTROL && !defined CONFIG_DM_USB &&	\
	 defined CONFIG_USB_GADGET
static struct dwc2_plat_otg_data
		nx_otg_data = CONFIG_USB_GADGET_REGS;

static void board_udc_probe(void)
{
	dwc2_udc_probe(&nx_otg_data);
}
#endif

#if defined CONFIG_VIDEO &&	\
	(defined CONFIG_SPL_BUILD ||	\
	(!defined CONFIG_OF_CONTROL && !defined CONFIG_DM))
#include <asm/arch/display.h>

#define DP_MODULE		0
#define DP_DEVICE_TYPE		DP_DEVICE_LVDS

static struct dp_lvds_dev dp_dev = {
	.lvds_format = 1,	/* 0: VESA, 1: JEIDA */
};

static struct nx_display_platdata dp_platdata = {
	.module = DP_MODULE,
	.sync  = {
		.pixel_clock_hz = 50000000,
		.h_active_len = 1024,
		.v_active_len = 600,
		.h_front_porch = 160,
		.h_back_porch = 140,
		.h_sync_width = 20,
		.v_front_porch = 12,
		.v_back_porch = 20,
		.v_sync_width = 3,
	},

	.ctrl  =  {
		.clk_src_lv0 = 3,
		.clk_div_lv0 = 16,
		.clk_src_lv1 = 7,
		.clk_div_lv1 = 1,
		.out_format = 2,
	},

	.top = {
		.screen_width = 1024,
		.screen_height = 600,
		.video_prior = 0,
		.back_color = 0x0,
	},

	.plane = {
		[0] = {
		.fb_base = 0x41000000,
		.width = 1024,
		.height = 600,
		.format = 0x06530000, /* 565: 0x44320000, A888: 0x06530000 */
		.pixel_byte = 4,
		},
	},

	.dev_type = DP_DEVICE_TYPE,
	.device = &dp_dev,
};
#endif

enum gpio_group {
	gpio_a,	gpio_b, gpio_c, gpio_d, gpio_e,
};

void board_display_probe(void)
{
#if defined CONFIG_SPL_BUILD ||	(	\
	!defined CONFIG_OF_CONTROL && !defined CONFIG_DM)
	nx_display_probe(&dp_platdata);
#endif
}

int board_init(void)
{
#if defined CONFIG_VIDEO
	/* set pwm0 output off: 1 */
	nx_gpio_set_pad_function(gpio_d,  1, 0);
	nx_gpio_set_output_value(gpio_d,  1, 1);
	nx_gpio_set_output_enable(gpio_d,  1, 1);
#endif

#ifdef CONFIG_SILENT_CONSOLE
	gd->flags |= GD_FLG_SILENT;
#endif

#if !defined CONFIG_OF_CONTROL && !defined CONFIG_DM_USB &&	\
	 defined CONFIG_USB_GADGET
	board_udc_probe();
#endif
#if defined CONFIG_VIDEO
	board_display_probe();
#endif
	return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
#ifdef CONFIG_SILENT_CONSOLE
	gd->flags &= ~GD_FLG_SILENT;
#endif

#if defined CONFIG_VIDEO
	/* set lcd enable */
	nx_gpio_set_pad_function(gpio_b, 25, 1);
	nx_gpio_set_pad_function(gpio_b, 27, 1);
	nx_gpio_set_pad_function(gpio_c, 11, 1);

	nx_gpio_set_output_value(gpio_b, 25, 1);
	nx_gpio_set_output_value(gpio_b, 27, 1);
	nx_gpio_set_output_value(gpio_c, 11, 1);

	nx_gpio_set_output_enable(gpio_b, 25, 1);
	nx_gpio_set_output_enable(gpio_b, 27, 1);
	nx_gpio_set_output_enable(gpio_c, 11, 1);
#endif

	return 0;
}
#endif

#ifdef CONFIG_SPL_BUILD
int spl_display_probe(void)
{
#if defined CONFIG_VIDEO
	board_display_probe();
#endif
	return 0;
}

int spl_late_init(void)
{
	return board_late_init();
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

