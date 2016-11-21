/*
 * (C) Copyright 2016 Nexell
 * Hyunseok, Jung <hsjung@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>

#ifdef CONFIG_DM_PMIC_NXE2000
#include <dm.h>
#include <dm/uclass-internal.h>
#include <power/pmic.h>
#include <power/nxe2000.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

enum gpio_group {
	gpio_a,	gpio_b, gpio_c, gpio_d, gpio_e,
};

#ifdef CONFIG_DM_PMIC_NXE2000
void pmic_init(void)
{
	static struct udevice *dev;
	int ret = -ENODEV;
	uint8_t bit_mask = 0;

	ret = pmic_get("nxe2000_gpio@32", &dev);
	if (ret)
		printf("Can't get PMIC: %s!\n", "nxe2000_gpio@32");

	bit_mask = pmic_reg_read(dev, NXE2000_REG_PWRONTIMSET);
	bit_mask &= ~(0x1 << NXE2000_POS_PWRONTIMSET_OFF_JUDGE_PWRON);
	bit_mask |= (0x0 << NXE2000_POS_PWRONTIMSET_OFF_JUDGE_PWRON);
	ret = pmic_write(dev, NXE2000_REG_PWRONTIMSET, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC REG: %d!\n", NXE2000_REG_PWRONTIMSET);

	bit_mask = 0x00;
	ret = pmic_reg_write(dev, (u32)NXE2000_REG_BANKSEL, (u32)bit_mask);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_BANKSEL);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(3 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(0 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC1CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC1CTL2);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(3 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(0 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC2CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC2CTL2);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC3CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC3CTL2);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC4CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC4CTL2);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC5CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC5CTL2);

	bit_mask = (1 << NXE2000_POS_CHGCTL1_SUSPEND);
	ret = pmic_write(dev, NXE2000_REG_CHGCTL1, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_CHGCTL1);
}
#endif

#ifdef LCD_POWER_INIT_OFF
static void board_backlight_init(void)
{
	/* PWM */
	nx_gpio_set_pad_function(gpio_d, 1, 0);
	nx_gpio_set_output_enable(gpio_d, 1, 1);
	nx_gpio_set_output_value(gpio_d, 1, 0);	/* LCD PWM */

	/* LED+, LED- */
	nx_gpio_set_pad_function(gpio_b, 27, 1);
	nx_gpio_set_output_enable(gpio_b, 27, 1);
	nx_gpio_set_output_value(gpio_b, 27, 0); 	/* LEDPWRON */

	nx_gpio_set_pad_function(gpio_e, 13, 0);
	nx_gpio_set_output_enable(gpio_e, 13, 1);
	nx_gpio_set_output_value(gpio_e, 13, 0);	/* LCD RESET */

	/* VCOM, AVDD, VGH, VGL */
	nx_gpio_set_pad_function(gpio_b, 25, 1);
	nx_gpio_set_output_enable(gpio_b, 25, 1);
	nx_gpio_set_output_value(gpio_b, 25, 0);	/* GATEPWRON */

	nx_gpio_set_pad_function(gpio_b, 26, 1);
	nx_gpio_set_output_enable(gpio_b, 26, 1);
	nx_gpio_set_output_value(gpio_b, 26, 0);	/* GATERST */

	/* VCC3P3_LCD */
	nx_gpio_set_pad_function(gpio_c, 11, 1);
	nx_gpio_set_output_enable(gpio_c, 11, 1);
	nx_gpio_set_output_value(gpio_c, 11, 0);	/* LCDPWREN */
}
#endif

static int board_backlight_on(void)
{
	/* VCOM, AVDD, VGH, VGL */
	nx_gpio_set_pad_function(gpio_b, 25, 1);
	nx_gpio_set_output_enable(gpio_b, 25, 1);
	nx_gpio_set_output_value(gpio_b, 25, 1);	/* GATEPWRON */

	nx_gpio_set_pad_function(gpio_b, 26, 1);
	nx_gpio_set_output_enable(gpio_b, 26, 1);
	nx_gpio_set_output_value(gpio_b, 26, 1);	/* GATERST */

	/* VCC3P3_LCD */
	nx_gpio_set_pad_function(gpio_c, 11, 1);
	nx_gpio_set_output_enable(gpio_c, 11, 1);
	nx_gpio_set_output_value(gpio_c, 11, 1);	/* LCDPWREN */

	/* LED+, LED- */
	nx_gpio_set_pad_function(gpio_b, 27, 1);
	nx_gpio_set_output_enable(gpio_b, 27, 1);
	nx_gpio_set_output_value(gpio_b, 27, 1);	/* LEDPWRON */

	nx_gpio_set_pad_function(gpio_e, 13, 0);
	nx_gpio_set_output_enable(gpio_e, 13, 1);
	nx_gpio_set_output_value(gpio_e, 13, 1);	/* LCD RESET */

	/* PWM */
	nx_gpio_set_pad_function(gpio_d, 1, 0);
	nx_gpio_set_output_enable(gpio_d, 1, 1);
	nx_gpio_set_output_value(gpio_d, 1, 0);		/* LCD PWM */

	return 0;
}

int board_init(void)
{
#ifdef CONFIG_DM_PMIC_NXE2000
	pmic_init();
#endif

#ifdef LCD_POWER_INIT_OFF
	board_backlight_init();
#endif
	// board_backlight_on();
	return 0;
}

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

int board_late_init(void)
{
	return 0;
}