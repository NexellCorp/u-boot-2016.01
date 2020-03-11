
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

#ifdef CONFIG_REVISION_TAG
#include <asm/arch/nx_adc.h>
#endif

#ifdef CONFIG_DM_PMIC_NXE2000
#include <dm.h>
#include <dm/uclass-internal.h>
#include <power/pmic.h>
#include <power/nxe2000.h>
#endif

#ifdef CONFIG_DM_REGULATOR_NXE2000
#include <power/regulator.h>
#endif
#include <nexell_common.h>

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

#ifdef CONFIG_REVISION_TAG
u32 board_rev;

u32 get_board_rev(void)
{
	return board_rev;
}

static void check_hw_revision(void)
{
	u32 val = 0;
	unsigned int adcval0, adcval1;

	/* adc ch 0 and 1 data */
	adcval0 = get_nexell_adc_val(0);
	if(adcval0 > 3000)
		val |= 1 << 1;
	adcval1 = get_nexell_adc_val(1);
	if(adcval1 > 3000)
		val |= 1 << 0;

	board_rev = val;
}

static void set_board_rev(u32 revision)
{
	char info[64] = {0, };

	snprintf(info, ARRAY_SIZE(info), "%d", revision);
	setenv("board_rev", info);
}
#endif

u32 cam_input;

static void check_cam_input(void)
{
	u32 val = 0;
	unsigned int adcval0, adcval1;

	/* adc ch 3 and 4 data */
	adcval0 = get_nexell_adc_val(3);
	if (adcval0 > 3000)
		val |= 1 << 1;
	adcval1 = get_nexell_adc_val(4);
	if (adcval1 > 3000)
		val |= 1 << 0;

	cam_input = val;
}

static void set_cam_input(u32 cam_input)
{
	char info[64] = {0, };

	snprintf(info, ARRAY_SIZE(info), "%d", cam_input);
	setenv("cam_input", info);
}

u32 rear_cam;

static void check_rear_cam(void)
{
	u32 val = 0;
	unsigned int adcval0, adcval1;

	/* adc ch 5 and 6 data */
	adcval0 = get_nexell_adc_val(5);
	if (adcval0 > 3000)
		val |= 1 << 1;
	adcval1 = get_nexell_adc_val(6);
	if (adcval1 > 3000)
		val |= 1 << 0;

	rear_cam = val;
}

static void set_rear_cam(u32 rear_cam)
{
	char info[64] = {0, };

	snprintf(info, ARRAY_SIZE(info), "%d", rear_cam);
	setenv("rear_cam", info);
}

u32 isSdCard_sub_board;

u32 get_SdCard_sub_board_status(void)
{
	return isSdCard_sub_board;
}

static void check_SdCard_sub_board_status(void)
{
	u32 val = 0;

	val = nx_gpio_get_input_value(gpio_d, 8);
	isSdCard_sub_board = val;
}

static void set_SdCard_sub_board_status(u32 sub_board)
{
	char info[64] = {0, };

	snprintf(info, ARRAY_SIZE(info), "%d", sub_board);
	setenv("sub_board", info);
}

int board_init(void)
{
#ifdef CONFIG_REVISION_TAG
	check_hw_revision();
#ifndef QUICKBOOT
	printf(" HW Revision: %d\n", board_rev);
#endif
#endif
	check_cam_input();
#ifndef QUICKBOOT
	printf("Camera input: \t%d\n", cam_input);
#endif
	check_rear_cam();
#ifndef QUICKBOOT
	printf("rear cam: \t%d\n", rear_cam);
#endif
	check_SdCard_sub_board_status();

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
#ifdef CONFIG_REVISION_TAG
	set_board_rev(board_rev);
#endif
	set_cam_input(cam_input);
	set_rear_cam(rear_cam);
	set_SdCard_sub_board_status(isSdCard_sub_board);
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

	run_fastboot_update();

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

#ifdef CONFIG_DM_PMIC_NXE2000
void power_init_board(void)
{
	struct udevice *pmic;
	struct udevice *dev;
#ifdef CONFIG_DM_REGULATOR
	struct dm_regulator_uclass_platdata *reg_uc_pdata;
	struct udevice *regulator;
#endif
#ifdef CONFIG_DM_CHARGER
	struct dm_charger_uclass_platdata *chg_uc_pdata;
	struct udevice *charger;
#endif
	int ret = -ENODEV;

	ret = pmic_get("nxe2000_gpio@32", &pmic);
	if (ret)
		printf("Can't get PMIC: %s!\n", "nxe2000_gpio@32");

	if (device_has_children(pmic)) {
#ifdef CONFIG_DM_REGULATOR
		for (ret = uclass_find_first_device(UCLASS_REGULATOR, &dev);
			dev;
			ret = uclass_find_next_device(&dev)) {
			if (ret)
				continue;

			reg_uc_pdata = dev_get_uclass_platdata(dev);
			if (!reg_uc_pdata)
				continue;

			uclass_get_device_tail(dev, 0, &regulator);
		}
#endif
#ifdef CONFIG_DM_CHARGER
		for (ret = uclass_find_first_device(UCLASS_CHARGER, &dev);
			dev;
			ret = uclass_find_next_device(&dev)) {
			if (ret)
				continue;

			chg_uc_pdata = dev_get_uclass_platdata(dev);
			if (!chg_uc_pdata)
				continue;

			uclass_get_device_tail(dev, 0, &charger);
		}

		/* charger_get_value_vbatt(charger); */
#endif
	}
}
#endif

