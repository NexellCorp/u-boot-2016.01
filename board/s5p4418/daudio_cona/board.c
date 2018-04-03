/*
 * (C) Copyright 2017 FMS
 * Han Yibeom <yibhan@cafeel.co.kr>
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
#include <pwm-nexell.h>

DECLARE_GLOBAL_DATA_PTR;

enum gpio_group {
	gpio_a,
	gpio_b,
	gpio_c,
	gpio_d,
	gpio_e,
};

int board_init(void)
{
/* set pwm0 output off: 1 */
#if !defined(CONFIG_TARGET_S5P4418_DAUDIO_CONA)
	nx_gpio_set_pad_function(gpio_d, 1, 0);
	nx_gpio_set_output_value(gpio_d, 1, 1);
	nx_gpio_set_output_enable(gpio_d, 1, 1);
#endif

#ifdef CONFIG_SILENT_CONSOLE
	gd->flags |= GD_FLG_SILENT;
#endif
#if defined(CONFIG_TARGET_S5P4418_DAUDIO_CONA)
	daudio_cona_gpio_default_setting();
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
#if !defined(CONFIG_TARGET_S5P4418_DAUDIO_CONA)
	nx_gpio_set_pad_function(gpio_c, 11, 1);
	nx_gpio_set_pad_function(gpio_b, 25, 1);
	nx_gpio_set_pad_function(gpio_b, 27, 1);

	nx_gpio_set_output_value(gpio_c, 11, 1);
	nx_gpio_set_output_value(gpio_b, 25, 1);
	nx_gpio_set_output_value(gpio_b, 27, 1);

	nx_gpio_set_output_enable(gpio_c, 11, 1);
	nx_gpio_set_output_enable(gpio_b, 25, 1);
	nx_gpio_set_output_enable(gpio_b, 27, 1);
#endif

#ifdef CONFIG_RECOVERY_BOOT
#define ALIVE_SCRATCH1_READ_REGISTER    (0xc00108b4)
#define ALIVE_SCRATCH1_RESET_REGISTER   (0xc00108ac)
#define RECOVERY_SIGNATURE              (0x52455343)    /* (ASCII) : R.E.S.C */
#ifndef QUICKBOOT
	printf("signature --> 0x%x\n", readl(ALIVE_SCRATCH1_READ_REGISTER));
#endif
	if (readl(ALIVE_SCRATCH1_READ_REGISTER) == RECOVERY_SIGNATURE) {
		printf("reboot recovery!!!!\n");
		writel(0xffffffff, ALIVE_SCRATCH1_RESET_REGISTER);
		setenv("bootcmd", "run recoveryboot");
	}
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
	gd->bd->bi_dram[0].size = CONFIG_SYS_SDRAM_SIZE;
}

void daudio_cona_gpio_default_setting(void)
{
	printf("yibhan:cona update check~~\n");
/*CPU_OK*/
	nx_gpio_set_pad_function(gpio_e, 18, 0);
	nx_gpio_set_output_value(gpio_e, 18, 1);
	nx_gpio_set_output_enable(gpio_e, 18, 1);
	/*CPU_OK_PLS*/
	nx_gpio_set_pad_function(gpio_d, 13, 0);
	nx_gpio_set_output_value(gpio_d, 13, 1);
	nx_gpio_set_output_enable(gpio_d, 13, 1);
	/*DNLD_SW*/
	nx_gpio_set_pad_function(gpio_e, 12, 0);
	nx_gpio_set_output_value(gpio_e, 12, 1);
	nx_gpio_set_output_enable(gpio_e, 12, 1);
	/*UART_SW*/
	nx_gpio_set_pad_function(gpio_e, 16, 0);
	nx_gpio_set_output_value(gpio_e, 16, 1);
	nx_gpio_set_output_enable(gpio_e, 16, 1);
	/*SYS_PWR*/
	nx_gpio_set_pad_function(gpio_e, 17, 0);
	nx_gpio_set_output_value(gpio_e, 17, 0);
	nx_gpio_set_output_enable(gpio_e, 17, 1);
	/*UCOM_RST*/
	nx_gpio_set_pad_function(gpio_e, 11, 0);
	nx_gpio_set_output_value(gpio_e, 11, 0);
	nx_gpio_set_output_enable(gpio_e, 11, 1);
	/*CTL1*/
	nx_gpio_set_pad_function(gpio_a, 17, 0);
	nx_gpio_set_output_value(gpio_a, 17, 1);
	nx_gpio_set_output_enable(gpio_a, 17, 1);
	/*CTL2*/
	nx_gpio_set_pad_function(gpio_a, 18, 0);
	nx_gpio_set_output_value(gpio_a, 18, 1);
	nx_gpio_set_output_enable(gpio_a, 18, 1);
	/*CTL3*/
	nx_gpio_set_pad_function(gpio_a, 19, 0);
	nx_gpio_set_output_value(gpio_a, 19, 0);
	nx_gpio_set_output_enable(gpio_a, 19, 1);
	/* LCD_RST */
	nx_gpio_set_pad_function(gpio_e, 7, 0);
	nx_gpio_set_output_value(gpio_e, 7, 1);
	nx_gpio_set_output_enable(gpio_e, 7, 1);
	/* HUB_RST */
	nx_gpio_set_pad_function(gpio_c, 3, 1);
	nx_gpio_set_output_value(gpio_c, 3, 1);
	nx_gpio_set_output_enable(gpio_c, 3, 1);
	/* TW9900_RST */
	nx_gpio_set_pad_function(gpio_b, 31, 1);
	nx_gpio_set_output_value(gpio_b, 31, 1);
	nx_gpio_set_output_enable(gpio_b, 31, 1);
}

#ifdef QUICKBOOT
#include <mmc.h>

#if 0
int board_set_mmc_pre(struct mmc *mmc)
{
	mmc->version = 1074069504;
	mmc->high_capacity = 1;
	mmc->bus_width = 4;
	mmc->clock = 50000000;
	mmc->card_caps = 0x7;
	mmc->ocr = 0xc0ff8080;
	mmc->dsr = 0xffffffff;
	mmc->dsr_imp = 0x0;
	mmc->scr[0] = 0x0;
	mmc->scr[1] = 0x0;
	mmc->csd[0] = 0xd0270132;
	mmc->csd[1] = 0x0f5903ff;
	mmc->csd[2] = 0xf6dbffef;
	mmc->csd[3] = 0x8e40400d;
	mmc->cid[0] = 0x15010038;
	mmc->cid[1] = 0x474e4433;
	mmc->cid[2] = 0x5201aaf8;
	mmc->cid[3] = 0x51b724ef;
	mmc->rca = 0x1;
	mmc->part_support = 7;
	mmc->part_num = 0;
	mmc->tran_speed = 52000000;
	mmc->read_bl_len = 512;
	mmc->write_bl_len = 512;
	mmc->erase_grp_size = 1024;
	mmc->hc_wp_grp_size = 16384;
	mmc->capacity = 7818182656;
	mmc->capacity_user = 7818182656;
	mmc->capacity_rpmb = 524288;
	mmc->capacity_gp[0] = 0;
	mmc->capacity_gp[1] = 0;
	mmc->capacity_gp[2] = 0;
	mmc->capacity_gp[3] = 0;
	mmc->enh_user_start = 0;
	mmc->enh_user_size = 0;
	mmc->ddr_mode = 0;

	return 1;
}
#endif
#endif
