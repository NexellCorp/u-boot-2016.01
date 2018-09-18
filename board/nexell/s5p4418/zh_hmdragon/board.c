/*
 * (C) Copyright 2018 Nexell
 * Jongshin, Park <pjsin865@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>

#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>

#ifdef CONFIG_DM_PMIC_NXE2000
#include <dm.h>
#include <dm/uclass-internal.h>
#include <power/pmic.h>
#include <power/nxe2000.h>
#endif

#ifdef CONFIG_DM_REGULATOR_NXE2000
#include <power/regulator.h>
#endif

#ifdef CONFIG_ADV7613
#include <adv7613.h>
#endif

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
	int gp = pwm_dev[CONFIG_BACKLIGHT_CH].grp;
	int io = pwm_dev[CONFIG_BACKLIGHT_CH].bit;
	int fn = pwm_dev[CONFIG_BACKLIGHT_CH].io_fn;

	/*
	 * pwm backlight OFF: HIGH, ON: LOW
	 */
	nx_gpio_set_pad_function(gp, io, fn);
	nx_gpio_set_output_value(gp, io, 0);
	nx_gpio_set_output_enable(gp, io, 1);
#endif
}

static void board_backlight_enable(void)
{
#ifdef CONFIG_PWM_NX
	/*
	 * pwm backlight ON: HIGH, ON: LOW
	 */
	pwm_init(
		CONFIG_BACKLIGHT_CH,
		CONFIG_BACKLIGHT_DIV, CONFIG_BACKLIGHT_INV
		);
	pwm_config(
		CONFIG_BACKLIGHT_CH,
		TO_DUTY_NS(CONFIG_BACKLIGHT_DUTY, CONFIG_BACKLIGHT_HZ),
		TO_PERIOD_NS(CONFIG_BACKLIGHT_HZ)
		);
#endif
}

static void board_gpio_ctl(int gp, int io, int fn, int on, u32 mode)
{
	nx_gpio_set_pad_function(gp, io, fn);

	if (on)
		nx_gpio_set_output_value(gp, io, 1);
	else
		nx_gpio_set_output_value(gp, io, 0);

	nx_gpio_set_pull_mode(gp, io, mode);
	nx_gpio_set_output_enable(gp, io, 1);
}

int board_init(void)
{
	board_backlight_disable();

#ifdef CONFIG_SERIAL_MCU
	nx_gpio_set_pad_function(gpio_d, 20, 1); /*UART2TXD */
	nx_gpio_set_output_enable(gpio_d, 20, 1);

	nx_gpio_set_pad_function(gpio_d, 16, 1); /* UART2RXD */
	nx_gpio_set_output_enable(gpio_d, 16, 1);
#endif

#ifdef CONFIG_SILENT_CONSOLE
	gd->flags |= GD_FLG_SILENT;
#endif

	return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	char *recovery_bootargs = malloc(512 * sizeof(char));;

#ifdef CONFIG_SILENT_CONSOLE
	gd->flags &= ~GD_FLG_SILENT;
#endif

	/* GSML_PWDN */
	//board_gpio_ctl(gpio_b, 26, 1, 1, nx_gpio_pull_off);

	/* CAM_POWER_EN */
	//board_gpio_ctl(gpio_b, 27, 1, 1, nx_gpio_pull_off);

	/* Not used : gpio, output, low */
	board_gpio_ctl(gpio_c,  9, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_c, 10, 1, 0, nx_gpio_pull_down);

	/* BT EN High:On */
	board_gpio_ctl(gpio_d, 28, 0, 1, nx_gpio_pull_off);

	/* WIFI EN High:Off */
	board_gpio_ctl(gpio_d, 29, 0, 0, nx_gpio_pull_off);

	/* gps EN High:Off */
	board_gpio_ctl(gpio_c, 4, 1, 1, nx_gpio_pull_off);

	/* Not used : gpio, output, low */
	board_gpio_ctl(gpio_c, 25, 1, 0, nx_gpio_pull_down);

	/* DIV_TP I2C : gpio, output, high */
	board_gpio_ctl(gpio_c, 29, 0, 1, nx_gpio_pull_off);

	/* USB0_CHG_EN */
	board_gpio_ctl(gpio_c, 30, 0, 1, nx_gpio_pull_off);

	/* USB_OTG_ID : output, low */
	board_gpio_ctl(gpio_d,  0, 0, 0, nx_gpio_pull_off);

	/* DIV_TP_RST : gpio, output, high */
	board_gpio_ctl(gpio_d, 11, 0, 1, nx_gpio_pull_off);

	/* SD2_PWREN Low:On*/
	board_gpio_ctl(gpio_e, 7, 0, 0, nx_gpio_pull_off);

	/* SD1_PWREN Low:On*/
	board_gpio_ctl(gpio_e, 13, 0, 0, nx_gpio_pull_off);

	/* DENOISE_PWEN */
	board_gpio_ctl(gpio_e, 14, 0, 1, nx_gpio_pull_off);

	/* DENOISE_RST */
	board_gpio_ctl(gpio_e, 15, 0, 1, nx_gpio_pull_off);

	/* Not used : gpio, output, low */
	board_gpio_ctl(gpio_e, 18, 0, 0, nx_gpio_pull_off);
	board_gpio_ctl(gpio_e, 19, 0, 0, nx_gpio_pull_off);

	/* Alive : gpio, input, pullup */

#ifdef CONFIG_USB_CHARGE
	usb_charge_init_s();
#endif

#ifdef CONFIG_ADV7613
	adv7613_init_s();
#endif
	board_backlight_enable();

#ifdef CONFIG_RECOVERY_BOOT
#define ALIVE_SCRATCH1_READ_REGISTER (0xc00108b4)
#define ALIVE_SCRATCH1_RESET_REGISTER (0xc00108ac)
#define RECOVERY_SIGNATURE (0x52455343) /* (ASCII) : R.E.S.C */
#ifndef QUICKBOOT
	printf("signature --> 0x%x\n", readl(ALIVE_SCRATCH1_READ_REGISTER));
#endif
	if (readl(ALIVE_SCRATCH1_READ_REGISTER) == RECOVERY_SIGNATURE) {
		printf("reboot recovery!!!!\n");
		writel(0xffffffff, ALIVE_SCRATCH1_RESET_REGISTER);
		recovery_bootargs = getenv("recovery_bootargs");
		setenv("bootargs", recovery_bootargs);
		setenv("bootcmd", "run recoveryboot");
		free(recovery_bootargs);
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
	}
}
#endif

#ifdef QUICKBOOT
#ifdef MMC_INIT_CANCEL
#include <mmc.h>
int board_set_mmc_pre(struct mmc *mmc)
{
    mmc->version        = 1074069504;
    mmc->high_capacity  = 1;
    mmc->bus_width      = 4;
    mmc->clock          = 100000000;    //50000000
    //mmc->clock          = 50000000;
    mmc->card_caps      = 0x7;
    mmc->ocr            = 0xc0ff8080;
    mmc->dsr            = 0xffffffff;
    mmc->dsr_imp        = 0x0;
    mmc->scr[0]         = 0x0;
    mmc->scr[1]         = 0x0;

    mmc->csd[0]         = 0xd0270032;
    mmc->csd[1]         = 0xf5903ff;
    mmc->csd[2]         = 0xffffffe7;
    mmc->csd[3]         = 0x8640009b;

    mmc->cid[0]         = 0x11010030;
    mmc->cid[1]         = 0x31364737;
    mmc->cid[2]         = 0x30002e2c;
    mmc->cid[3]         = 0x74ef6459;

    mmc->rca            = 0x1;
    mmc->part_support   = 7;
    mmc->part_num       = 0;
    mmc->tran_speed     = 100000000;    //52000000
    //mmc->tran_speed     = 52000000;
    mmc->read_bl_len    = 512;
    mmc->write_bl_len   = 512;
    mmc->erase_grp_size = 1024;
    mmc->hc_wp_grp_size = 8192;
    mmc->capacity       = 15758000128;
    mmc->capacity_user  = 15758000128;
    mmc->capacity_rpmb  = 4194304;
    mmc->capacity_gp[0] = 0;
    mmc->capacity_gp[1] = 0;
    mmc->capacity_gp[2] = 0;
    mmc->capacity_gp[3] = 0;
    mmc->enh_user_start = 0;
    mmc->enh_user_size  = 0;
    mmc->ddr_mode       = 0;
}
#endif
#endif


