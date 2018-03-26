/*
 * (C) Copyright 2017 Nexell
 * Sungwoo, Park <swpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <asm/gpio.h>
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
#ifdef CONFIG_DM_CHARGER
#include <power/charger.h>
#endif
#ifdef CONFIG_I2C_EEPROM
#include <i2c_eeprom.h>
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
	 * LCD LED Enable(GPA20): LOW
	 */
	nx_gpio_set_pad_function(gpio_a, 20, 0);
	nx_gpio_set_output_value(gpio_a, 20, 0);
	nx_gpio_set_output_enable(gpio_a, 20, 1);

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

#if 0
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
	/*
	 * LCD LED Enable(GPA20): HIGH
	*/
	nx_gpio_set_output_value(gpio_a, 20, 1);
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

int check_powerkey(struct udevice *dev_charger)
{
	struct gpio_desc gpio = {};
	int node;
	int key_value;
	int charger_status;
	char online;

	run_command("ext4load mmc 0:2 48000000 media/Charging.bmp B71E6",0);
	run_command("bmp display 48000000 150 390", 0);

	node = fdt_node_offset_by_compatible(gd->fdt_blob, 0,
		"nexell,board-pwrkey");

	if (node < 0) {
		printf("Cannot Founc board-pwrkey \n");
		return -1;
	}

	gpio_request_by_name_nodev(gd->fdt_blob, node, "pwrkey-gpio", 0, &gpio,
								GPIOD_IS_IN);

	if (!dm_gpio_is_valid(&gpio)) {
		printf("pwrkey-gpio not valid \n");
		return -1;
	}

	while(1) {
		key_value = (*(unsigned int *)(0xC001091C)) & 0x1;
		if(!key_value) {
			printf("power key pressed \n");
			return 1;
		}
		charger_status = charger_get_charge_type(dev_charger);
		online = (charger_status >> 2) & 0x01;
		if(!online)
			return -1;
		mdelay(500);
	}

}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	int ret = -ENODEV;
	uint32_t reset_signature;
#ifdef CONFIG_DM_CHARGER
	struct udevice *dev_charger;
	int charger_status, vbatt_value;
	char online;
#endif

#ifdef CONFIG_DM_PMIC_NXE2000
	struct udevice *dev_pmic;
#endif

#ifdef CONFIG_I2C_EEPROM
	struct udevice *dev;
	uint8_t buf[CONFIG_DBG_UNLOCK_SIZE] = {0, };
	uint32_t data;
	char *penv;
	char *ptr;
	char env_buf[256] = {0, };
#endif

#ifdef CONFIG_SILENT_CONSOLE
	gd->flags &= ~GD_FLG_SILENT;
#endif

	/* Temporary disable the CHARGER_ENABLE */
	nx_gpio_set_pad_function(gpio_a, 13, 0);
	nx_gpio_set_pull_mode(gpio_a, 13, 1);
	nx_gpio_set_output_enable(gpio_a, 13, 1);
	nx_gpio_set_output_value(gpio_a,13, 0);

	board_backlight_enable();

#ifdef CONFIG_SYS_BURNING
	setenv("bootcmd", "fastboot 0");
	run_command("run bootcmd", 0);
#endif


#ifdef CONFIG_DM_CHARGER
	/* Check Battery */
	ret = uclass_get_device_by_name(UCLASS_CHARGER, "bq25895m@6a", &dev_charger);
	if (ret < 0) {
		printf("Cannot find charger device\n");
		return 0;
	}

	charger_status = charger_get_charge_type(dev_charger);

	online = (charger_status >> 2) & 0x01;

	printf("charger_status = 0x%x \n", charger_status);
	printf("online = 0x%x \n", online);

	if(!online) {
		printf("AC adaptor Not detected \n");
		goto power_off;
	} else {
		printf("AC adaptor detected \n");
	}

#endif /* CONFIG_DM_CHARGER */


#ifdef CONFIG_RECOVERY_BOOT
#define ALIVE_SCRATCH1_READ_REGISTER	(0xc00108b4)
#define ALIVE_SCRATCH1_RESET_REGISTER	(0xc00108ac)
#define RECOVERY_SIGNATURE				(0x52455343)    /* (ASCII) : R.E.S.C */
#define CHARGING_SIGNATURE				(0x43484152)    /* (ASCII) : C.H.A.G */
	reset_signature = readl(ALIVE_SCRATCH1_READ_REGISTER);
	printf("signature --> 0x%x\n", reset_signature);
	switch (reset_signature) {
		case RECOVERY_SIGNATURE :
			printf("reboot recovery!!!!\n");
			writel(0xffffffff, ALIVE_SCRATCH1_RESET_REGISTER);
			setenv("bootcmd", "run recoveryboot");
			break;
		case CHARGING_SIGNATURE :
			printf("Charging !!!!\n");
			writel(0xffffffff, ALIVE_SCRATCH1_RESET_REGISTER);
			ret = check_powerkey(dev_charger);
			if (ret < 0)
				goto power_off;
		default :
			writel(0xffffffff, ALIVE_SCRATCH1_RESET_REGISTER);
			break;

	}
#endif

#ifdef CONFIG_DM_CHARGER
	vbatt_value = charger_get_value_vbatt(dev_charger);
	printf("vbatt_value = %d \n", vbatt_value);
	if(vbatt_value < 3600) {
		printf("LOW vbatt(%d) \n", vbatt_value);
		ret = check_powerkey(dev_charger);
		if (ret < 0)
			goto power_off;
	}
#endif /* CONFIG_DM_CHARGER */



#ifdef CONFIG_I2C_EEPROM
	ret = uclass_get_device_by_name(UCLASS_I2C_EEPROM, "eeprom", &dev);
	if (ret < 0) {
		printf("Cannot find eeprom device\n");
		goto dbg_lock;
	}

	dm_eeprom_read(dev, CONFIG_DBG_UNLOCK_OFFSET, &buf[0],
						CONFIG_DBG_UNLOCK_SIZE);
	printf("EEPROM#:#1:0x%X, #2:0x%X, #3:0x%X, #4:0x%X\n",
		buf[0], buf[1], buf[2], buf[3]);
	memcpy(&data,buf,sizeof(data));

	if(data == CONFIG_DBG_UNLOCK_KEY) {
		printf("DBG UNLOCKED \n");
		penv = getenv("bootargs");
		ptr = strstr(penv,CONFIG_DBG_UNLOCK_ARG);
		if(ptr != NULL)
			sprintf(env_buf,"%s", penv);
		else
			sprintf(env_buf,"%s %s", penv, CONFIG_DBG_UNLOCK_ARG);
		setenv("bootargs",env_buf);
		return 0;
	}

dbg_lock:
	printf("DBG LOCKED \n");
	penv = getenv("bootargs");
	sprintf(env_buf,"%s %s", penv, CONFIG_DBG_LOCK_ARG);
	setenv("bootargs",env_buf);
	run_command("run bootcmd", 0);
	return 0;

power_off:
#ifdef CONFIG_DM_PMIC_NXE2000
	ret = pmic_get("nxe2000_gpio@32", &dev_pmic);
	if (ret) {
		printf("Can't get PMIC: %s!\n", "nxe2000_gpio@32");
		return 0;
	}
	printf("Power Off\n");
	mdelay(500);
	pmic_reg_write(dev_pmic, (u32)NXE2000_REG_SLPCNT, 1);
#endif

#endif

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
#endif
	}
}
#endif
