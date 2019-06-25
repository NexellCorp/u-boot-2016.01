/*
 * (C) Copyright 2016 Nexell
 * Hyejung Kwon  <cjscld15@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
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
#ifdef CONFIG_PWM_NX
#include <pwm.h>
#endif
#include <asm/io.h>

#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>

#include <asm-generic/gpio.h>
#include <nx_i2c_gpio.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_BOARD_EARLY_INIT_F
static void nx_eth_init(void)
{
	nx_gpio_set_pad_function  (gpio_e,  7, 1);     /* TXD0 */
	nx_gpio_set_pull_mode     (gpio_e,  7, 2);
	nx_gpio_set_drive_strength(gpio_e,  7, 3);
	nx_gpio_set_pad_function  (gpio_e,  8, 1);     /* TXD1 */
	nx_gpio_set_pull_mode     (gpio_e,  8, 2);
	nx_gpio_set_drive_strength(gpio_e,  8, 3);
	nx_gpio_set_pad_function  (gpio_e,  9, 1);     /* TXD2 */
	nx_gpio_set_pull_mode     (gpio_e,  9, 2);
	nx_gpio_set_drive_strength(gpio_e,  9, 3);
	nx_gpio_set_pad_function  (gpio_e, 10, 1);     /* TXD3 */
	nx_gpio_set_pull_mode     (gpio_e, 10, 2);
	nx_gpio_set_drive_strength(gpio_e, 10, 3);

	nx_gpio_set_pad_function  (gpio_e, 11, 1);     /* TXEN */
	nx_gpio_set_pull_mode     (gpio_e, 11, 2);
	nx_gpio_set_drive_strength(gpio_e, 11, 3);
	nx_gpio_set_pad_function  (gpio_e, 24, 1);     /* TXCK */
	nx_gpio_set_pull_mode     (gpio_e, 24, 2);
	nx_gpio_set_drive_strength(gpio_e, 24, 3);

	nx_gpio_set_pad_function  (gpio_e, 14, 1);     /* RXD0 */
	nx_gpio_set_pull_mode     (gpio_e, 14, 2);
	nx_gpio_set_drive_strength(gpio_e, 14, 3);
	nx_gpio_set_pad_function  (gpio_e, 15, 1);     /* RXD1 */
	nx_gpio_set_pull_mode     (gpio_e, 15, 2);
	nx_gpio_set_drive_strength(gpio_e, 15, 3);
	nx_gpio_set_pad_function  (gpio_e, 16, 1);     /* RXD2 */
	nx_gpio_set_pull_mode     (gpio_e, 16, 2);
	nx_gpio_set_drive_strength(gpio_e, 16, 3);
	nx_gpio_set_pad_function  (gpio_e, 17, 1);     /* RXD3 */
	nx_gpio_set_pull_mode     (gpio_e, 17, 2);
	nx_gpio_set_drive_strength(gpio_e, 17, 3);

	nx_gpio_set_pad_function  (gpio_e, 19, 1);     /* RXDV */
	nx_gpio_set_pull_mode     (gpio_e, 19, 2);
	nx_gpio_set_drive_strength(gpio_e, 19, 3);
	nx_gpio_set_pad_function  (gpio_e, 18, 1);     /* RXCK */
	nx_gpio_set_pull_mode     (gpio_e, 18, 2);
	nx_gpio_set_drive_strength(gpio_e, 18, 3);

	nx_gpio_set_pad_function  (gpio_e, 20, 1);     /* MDC  */
	nx_gpio_set_pull_mode     (gpio_e, 20, 2);
	nx_gpio_set_drive_strength(gpio_e, 20, 3);
	nx_gpio_set_pad_function  (gpio_e, 21, 1);     /* MDIO */
	nx_gpio_set_pull_mode     (gpio_e, 21, 2);
	nx_gpio_set_drive_strength(gpio_e, 21, 3);

	nx_gpio_set_pad_function  (gpio_e, 22, 0);     /* RST  */
	nx_gpio_set_pad_function  (gpio_e, 23, 0);     /* INT  */

	nx_gpio_set_output_enable(gpio_e, 22, 1);
	nx_gpio_set_output_value(gpio_e, 22, 1);
}

/* call from u-boot */
int board_early_init_f(void)
{
	nx_gpio_set_pad_function  (gpio_a,  4, 0);     /* LED 0  */
	nx_gpio_set_pull_mode     (gpio_a,  4, 2);
	nx_gpio_set_drive_strength(gpio_a,  4, 0);
	nx_gpio_set_output_enable (gpio_a,  4, 1);
	nx_gpio_set_output_value  (gpio_a,  4, 0);

	nx_gpio_set_pad_function  (gpio_a,  5, 0);     /* LED 1  */
	nx_gpio_set_pull_mode     (gpio_a,  5, 2);
	nx_gpio_set_drive_strength(gpio_a,  5, 0);
	nx_gpio_set_output_enable (gpio_a,  5, 1);
	nx_gpio_set_output_value  (gpio_a,  5, 0);

	nx_gpio_set_pad_function  (gpio_a,  3, 0);     /* Key   */
	nx_gpio_set_pull_mode     (gpio_a,  3, 2);
	nx_gpio_set_drive_strength(gpio_a,  3, 3);
	nx_gpio_set_output_enable (gpio_a,  3, 0);

	nx_gpio_set_pad_function  (gpio_d, 31, 0);	// AP_HASH0_RST
	nx_gpio_set_pull_mode     (gpio_d, 31, 2);
	nx_gpio_set_drive_strength(gpio_d, 31, 3);
	nx_gpio_set_output_enable (gpio_d, 31, 1);
	nx_gpio_set_output_value  (gpio_d, 31, 0);	// reset

	nx_gpio_set_pad_function  (gpio_a, 24, 0);	// AP_HASH0_PLUG
	nx_gpio_set_pull_mode     (gpio_a, 24, nx_gpio_pull_up);
	nx_gpio_set_drive_strength(gpio_a, 24, 3);
	nx_gpio_set_output_enable (gpio_a, 24, 1);

	nx_gpio_set_pad_function  (gpio_e,  4, 0);	// AP_HASH1_RST
	nx_gpio_set_pull_mode     (gpio_e,  4, 2);
	nx_gpio_set_drive_strength(gpio_e,  4, 3);
	nx_gpio_set_output_enable (gpio_e,  4, 0);
	nx_gpio_set_output_value  (gpio_e,  4, 0);	// reset

	nx_gpio_set_pad_function  (gpio_a, 11, 0);	// AP_HASH1_PLUG
	nx_gpio_set_pull_mode     (gpio_a, 11, nx_gpio_pull_up);
	nx_gpio_set_drive_strength(gpio_a, 11, 3);
	nx_gpio_set_output_enable (gpio_a, 11, 0);

       nx_eth_init();
       return 0;
}
#endif

int board_init(void)
{
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

#ifdef CONFIG_RECOVERY_BOOT
#define ALIVE_SCRATCH1_READ_REGISTER	(0xc00108b4)
#define ALIVE_SCRATCH1_RESET_REGISTER	(0xc00108ac)
#define RECOVERY_SIGNATURE				(0x52455343)    /* (ASCII) : R.E.S.C */
	printf("signature --> 0x%x\n", readl(ALIVE_SCRATCH1_READ_REGISTER));
	if (readl(ALIVE_SCRATCH1_READ_REGISTER) == RECOVERY_SIGNATURE) {
		printf("reboot recovery!!!!\n");
		writel(0xffffffff, ALIVE_SCRATCH1_RESET_REGISTER);
		setenv("bootcmd", "run recoveryboot");
	}
#endif

	return 0;
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
