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
#include <nx_i2c_gpio.h>

DECLARE_GLOBAL_DATA_PTR;

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

static u8 camera_sensor_reg[] = {0x02, 0x1c, 0x03, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x10, 0x11, 0x19, 0x1b, 0x1a, 0xaf, 0xb1};
static u8 camera_sensor_val[] = {0x40, 0x00, 0xa6, 0x80, 0x02, 0x15, 0xf0, 0x09, 0xc0, 0xec, 0x68, 0x50, 0x00, 0x0f, 0x40, 0x20};

int board_rearcam_check(void)
{
	int rearcam_on = 0;
	unsigned int gpio;
	int ret;

#if defined(CONFIG_DM_GPIO)
	ret = gpio_lookup_name("gpio_alv0", NULL, NULL, &gpio);
	if (ret)
		printf("gpio_lookup_name: pin gpio_alv0 3 failed\n");
#else
	gpio = name_to_gpio("gpio_alv0");
	if (gpio < 0)
		printf("name_to_gpio: pin gpio_alv0 3 failed\n");
#endif
	gpio += 3;
	ret = gpio_request(gpio, "rearcam_detect_gpio");
	if (ret) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return -1;
	}
	gpio_direction_input(gpio);
	rearcam_on = (gpio_get_value(gpio)) ? 0 : 1;
	gpio_free(gpio);
	return rearcam_on;
}

static int board_camera_sensor_power_enable(void)
{
	nx_gpio_set_pad_function(gpio_e, 12, 1);
	nx_gpio_set_pad_function(gpio_e, 16, 1);
	nx_gpio_set_pad_function(gpio_c, 9, 1);

	nx_gpio_set_output_value(gpio_e, 12, 0);
	nx_gpio_set_output_value(gpio_e, 16, 0);
	nx_gpio_set_output_value(gpio_c, 9, 0);

	nx_gpio_set_output_enable(gpio_e, 12, 1);
	nx_gpio_set_output_value(gpio_e, 12, 0);
	udelay(10000);
	nx_gpio_set_output_enable(gpio_e, 16, 1);
	nx_gpio_set_output_value(gpio_e, 16, 0);
	udelay(10000);
	nx_gpio_set_output_enable(gpio_c, 9, 1);
	nx_gpio_set_output_value(gpio_c, 9, 1);
	udelay(10000);
	nx_gpio_set_output_enable(gpio_e, 16, 1);
	nx_gpio_set_output_enable(gpio_c, 9, 1);

	return 0;
}

static int board_camera_sensor_init(void)
{
	struct udevice *dev = NULL;
	int ret = 0, i = 0;

	dev = nx_i2c_gpio_init();
	if (!dev) {
		printf("failed to get nx i2c gpio device\n");
		ret = -1;
		goto exit;
	}

	for (i = 0; i < ARRAY_SIZE(camera_sensor_reg); i++) {
		ret = nx_i2c_gpio_write(dev, camera_sensor_reg[i],
				&camera_sensor_val[i],
				1);
		if (ret)
			goto exit;
	}
exit:
	if (ret) {
		printf("failed to init camera sensor:%d\n", ret);
	}

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

	/*
	 * set lcd enable
	 */
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

int board_init(void)
{
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

#ifdef CONFIG_SILENT_CONSOLE
	gd->flags &= ~GD_FLG_SILENT;
#endif

#ifdef QUICKBOOT
	board_camera_sensor_power_enable();
	board_camera_sensor_init();
#endif

#ifndef CONFIG_VIDEO_LOGO
    udelay(25000);
#endif
	board_backlight_enable();

#ifdef CONFIG_RECOVERY_BOOT
#define ALIVE_SCRATCH1_READ_REGISTER	(0xc00108b4)
#define ALIVE_SCRATCH1_RESET_REGISTER	(0xc00108ac)
#define RECOVERY_SIGNATURE				(0x52455343)    /* (ASCII) : R.E.S.C */
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
	mmc->cid[2] = 0x5201aaf9;
	mmc->cid[3] = 0x5051245b;
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
	mmc->enh_user_start = 8;
	mmc->enh_user_size = 8;
	mmc->ddr_mode = 0;

	return 1;
}
#endif

#endif
