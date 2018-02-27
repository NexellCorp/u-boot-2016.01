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
#include <asm/arch/nx_gpio.h>

#include <asm-generic/gpio.h>
#include <nexell_i2c.h>

#ifdef CONFIG_DM_PMIC_NXE2000
#include <dm.h>
#include <dm/uclass-internal.h>
#include <power/pmic.h>
#include <power/nxe2000.h>
#endif

#ifdef CONFIG_DM_REGULATOR_NXE2000
#include <power/regulator.h>
#endif

#ifdef CONFIG_USB_CHARGE
#include <usb_charge.h>
#endif

#ifdef CONFIG_UB927928
#include <ub927928.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#define ALIVE_PWRGATEREG		0x00
#define ALIVE_PADOUTEN_RST		0x74
#define ALIVE_PADOUTEN			0x78
#define ALIVE_PADOUTEN_READ		0x7C
#define ALIVE_PADOUTSET_RST		0x8C
#define ALIVE_PADOUTSET			0x90
#define ALIVE_PADOUTSET_READ	0x94

#ifdef CONFIG_PWM_NX
#include <pwm.h>

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

#ifdef QUICKBOOT
static u8 camera_sensor_reg[] = {0x40, 0x07, 0x0b, 0x39, 0x4d, 0x4e, 0xc8, 0x73, 0xb9, 0x4e, 0x02, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1d, 0x0c, 0x0d, 0x20, 0x26, 0x2b, 0x2d, 0x2d, 0x2e, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0xff};
static u8 camera_sensor_val[] = {0x00, 0xc0, 0xc0, 0x8c, 0x03, 0x17, 0x21, 0x01, 0x01, 0x17, 0xcf, 0x13, 0x4e, 0x80, 0x13, 0xf0, 0x07,0x09, 0x38, 0x53, 0x10, 0xa0, 0x12, 0x70, 0x70, 0x68, 0x5e, 0x62, 0xbb, 0x96, 0xc0, 0x25, 0x84, 0xff};
static const char boot_args[] = "setenv console=ttyAMA3,115200n8 loglevel=7 printk.time=1 androidboot.hardware=zh_blackdragon androidboot.console=ttyAMA3 androidboot.serialno=s5p4418_zh_blackdragon nx_rearcam.sensor_init_parm=1 quiet androidboot.selinux=permissive";

int board_rearcam_check(void)
{
	int rearcam_on = 0;

	nx_gpio_set_pad_function(gpio_b, 11, 1);
	nx_gpio_set_pull_mode(gpio_b, 11, nx_gpio_pull_down);
	nx_gpio_set_output_enable(gpio_b, 11, 0);
	nx_gpio_set_detect_enable(gpio_b, 11, 1);
	rearcam_on = !nx_gpio_get_input_value(gpio_b, 11);

	return rearcam_on;
}

static int board_camera_sensor_init(void)
{
	nx_i2c_init_s(camera_sensor_reg, camera_sensor_val);
	run_command_list(boot_args, sizeof(boot_args), 0);
	return 0;
}

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
	nx_gpio_set_output_value(gp, io, 1);
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

void board_gpio_init(void)
{
	nx_gpio_initialize();
	nx_gpio_set_base_address(0, (void *)PHY_BASEADDR_GPIOA);
	nx_gpio_set_base_address(1, (void *)PHY_BASEADDR_GPIOB);
	nx_gpio_set_base_address(2, (void *)PHY_BASEADDR_GPIOC);
	nx_gpio_set_base_address(3, (void *)PHY_BASEADDR_GPIOD);
	nx_gpio_set_base_address(4, (void *)PHY_BASEADDR_GPIOE);
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

static void board_gpio_alv_ctl(char *str_gpio, int on)
{
	unsigned int gpio;
	int ret;

#if defined(CONFIG_DM_GPIO)
	ret = gpio_lookup_name(str_gpio, NULL, NULL, &gpio);
	if (ret)
		printf("gpio_lookup_name: pin %s failed\n", str_gpio);
#else
	gpio = name_to_gpio(str_gpio);
	if (gpio < 0)
		printf("name_to_gpio: pin %s failed\n", str_gpio);
#endif

	ret = gpio_request(gpio, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return;
	}

	if (on)
		gpio_direction_output(gpio, 1);
	else
		gpio_direction_output(gpio, 0);

}

int board_init(void)
{
	board_gpio_init();

	board_gpio_alv_ctl("gpio_alv0", 0);

	board_backlight_disable();

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

	/* Not used : gpio, output, low*/
	board_gpio_ctl(gpio_b,  2, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_b,  4, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_b,  8, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_b, 10, 0, 0, nx_gpio_pull_down);

	/* Not used : gpio, output, low*/
	board_gpio_ctl(gpio_b, 12, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_b, 18, 2, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_b, 23, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_b, 24, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_b, 25, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_b, 26, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_b, 27, 1, 0, nx_gpio_pull_down);

	/* MFI I2C : gpio, output, high */
	board_gpio_ctl(gpio_b, 28, 2, 1, nx_gpio_pull_off);
	board_gpio_ctl(gpio_b, 29, 2, 1, nx_gpio_pull_off);

	/* Not used : gpio, output, low */
	board_gpio_ctl(gpio_c,  0, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_c,  1, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_c,  3, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_c,  4, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_c,  7, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_c,  8, 1, 0, nx_gpio_pull_down);

	/* LCD : gpio, output, HIGH */
	board_gpio_ctl(gpio_c,  9, 1, 1, nx_gpio_pull_off);
	board_gpio_ctl(gpio_c, 10, 1, 1, nx_gpio_pull_off);

	/* BT EN High:On */
	board_gpio_ctl(gpio_c, 11, 1, 1, nx_gpio_pull_off);

	/* WIFI EN High:On */
	board_gpio_ctl(gpio_c, 12, 1, 1, nx_gpio_pull_off);

	/* Not used : gpio, output, low */
	board_gpio_ctl(gpio_c, 14, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_c, 15, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_c, 16, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_c, 17, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_c, 24, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_c, 27, 1, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_c, 28, 0, 0, nx_gpio_pull_down);

	/* USB1 EN : gpio, output, high */
	board_gpio_ctl(gpio_c, 30, 0, 1, nx_gpio_pull_off);

	/* USB_OTG_POW_CTL : gpio, output, high */
	board_gpio_ctl(gpio_d,  0, 0, 1, nx_gpio_pull_off);

	/* USB_OTG_POW_CTL : gpio, output, high */
	board_gpio_ctl(gpio_d,  0, 0, 1, nx_gpio_pull_off);

	/* Not used : gpio, output, low */
	board_gpio_ctl(gpio_d,  1, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_d, 11, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_d, 13, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_d, 22, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_d, 23, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_d, 24, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_d, 25, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_d, 26, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_d, 27, 0, 0, nx_gpio_pull_down);

	/* SD2_PWREN High:Off*/
	board_gpio_ctl(gpio_e, 7, 0, 1, nx_gpio_pull_off);

	/* Not used : gpio, output, low */
	board_gpio_ctl(gpio_e, 11, 0, 0, nx_gpio_pull_down);
	board_gpio_ctl(gpio_e, 12, 0, 0, nx_gpio_pull_down);

	/* SD1_PWREN High:Off*/
	board_gpio_ctl(gpio_e, 13, 0, 1, nx_gpio_pull_off);

	/* GPS_POW_CTL : gpio, output, high */
	board_gpio_ctl(gpio_e, 14, 0, 1, nx_gpio_pull_off);

	/* USB2_CTL : gpio, output, high */
	board_gpio_ctl(gpio_e, 15, 0, 1, nx_gpio_pull_off);

	/* USB3_CTL : gpio, output, high */
	board_gpio_ctl(gpio_e, 18, 0, 1, nx_gpio_pull_off);

	/* USB4_CTL : gpio, output, high */
	board_gpio_ctl(gpio_e, 19, 0, 1, nx_gpio_pull_off);

	/* Alive : output */
	/* 927_PDB : AliveGPIO0 output, high */
	board_gpio_alv_ctl("gpio_alv0", 1);

	/* ARM_TP_RST : AliveGPIO5 output, low */
	board_gpio_alv_ctl("gpio_alv5", 0);

#ifdef CONFIG_UB927928
	ub927928_init_s();
#endif

#ifdef QUICKBOOT
	board_camera_sensor_init();
#endif
	board_backlight_enable();

#ifdef CONFIG_USB_CHARGE
	usb_charge_init_s();
#endif

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
#include <mmc.h>

int board_set_mmc_pre(struct mmc *mmc)
{
	mmc->has_init = 1;
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
	mmc->csd[0] = 0xd0270032;
	mmc->csd[1] = 0xf5903ff;
	mmc->csd[2] = 0xffffffe7;
	mmc->csd[3] = 0x8640009b;
	mmc->cid[0] = 0x11010030;
	mmc->cid[1] = 0x31364737;
	mmc->cid[2] = 0x30004a3a;
	mmc->cid[3] = 0x6ccba3b7;
	mmc->rca = 0x1;
	mmc->part_support = 7;
	mmc->part_num = 0;
	mmc->tran_speed = 52000000;
	mmc->read_bl_len = 512;
	mmc->write_bl_len = 512;
	mmc->erase_grp_size = 1024;
	mmc->hc_wp_grp_size = 8192;
	mmc->capacity = 15758000128;
	mmc->capacity_user = 15758000128;
	mmc->capacity_rpmb = 4194304;
	mmc->capacity_gp[0] = 0;
	mmc->capacity_gp[1] = 0;
	mmc->capacity_gp[2] = 0;
	mmc->capacity_gp[3] = 0;
	mmc->enh_user_start = 8;
	mmc->enh_user_size = 8;
	mmc->ddr_mode = 0;

	return 1;
}
#endif
