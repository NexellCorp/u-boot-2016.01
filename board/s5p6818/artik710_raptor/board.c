/*
 * (C) Copyright 2016 Nexell
 * Hyunseok, Jung <hsjung@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>

#include <asm/arch/s5p6818.h>
#include <asm/arch/nx_gpio.h>

#ifdef CONFIG_USB_GADGET
#include <usb.h>
#include <usb/dwc2_udc.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

/*------------------------------------------------------------------------------
 * intialize nexell soc and board status.
 */

static void nx_phy_init(void)
{
	/* Set interrupt config */
	nx_gpio_set_pad_function(0, 9, 0);
	nx_gpio_set_output_enable(0, 9, 1);
	nx_gpio_set_pad_function(0, 10, 0);	/* GPIO */

	/* PHY reset */
	nx_gpio_set_output_value(0, 10, 1);
	nx_gpio_set_output_enable(0, 10, 1);

	/* ALT FUNCTION - GMAC */
	nx_gpio_set_pad_function(4, 7, 1);
	nx_gpio_set_pad_function(4, 8, 1);
	nx_gpio_set_pad_function(4, 9, 1);
	nx_gpio_set_pad_function(4, 10, 1);
	nx_gpio_set_pad_function(4, 11, 1);
	nx_gpio_set_pad_function(4, 14, 1);
	nx_gpio_set_pad_function(4, 15, 1);
	nx_gpio_set_pad_function(4, 16, 1);
	nx_gpio_set_pad_function(4, 17, 1);
	nx_gpio_set_pad_function(4, 18, 1);
	nx_gpio_set_pad_function(4, 19, 1);
	nx_gpio_set_pad_function(4, 20, 1);
	nx_gpio_set_pad_function(4, 21, 1);
	nx_gpio_set_pad_function(4, 24, 1);

	/* PAD STRENGTH */
	nx_gpio_set_drive_strength(4, 7, 3);
	nx_gpio_set_drive_strength(4, 8, 3);
	nx_gpio_set_drive_strength(4, 9, 3);
	nx_gpio_set_drive_strength(4, 10, 3);
	nx_gpio_set_drive_strength(4, 11, 3);
	nx_gpio_set_drive_strength(4, 14, 3);
	nx_gpio_set_drive_strength(4, 15, 3);
	nx_gpio_set_drive_strength(4, 16, 3);
	nx_gpio_set_drive_strength(4, 17, 3);
	nx_gpio_set_drive_strength(4, 18, 0);	/* RX clk */
	nx_gpio_set_drive_strength(4, 19, 3);
	nx_gpio_set_drive_strength(4, 20, 3);
	nx_gpio_set_drive_strength(4, 21, 3);
	nx_gpio_set_drive_strength(4, 24, 3);	/* TX clk */
}

/* call from u-boot */
int board_early_init_f(void)
{
	return 0;
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

int board_init(void)
{
	board_gpio_init();

	nx_phy_init();

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

#ifdef CONFIG_USB_GADGET
struct dwc2_plat_otg_data s5p6818_otg_data = {
	.phy_control	= NULL,
	.regs_phy	= PHY_BASEADDR_TIEOFF,
	.regs_otg	= PHY_BASEADDR_HSOTG,
	.usb_phy_ctrl	= NULL,
	.usb_flags	= NULL,
};

int board_usb_init(int index, enum usb_init_type init)
{
	debug("USB_udc_probe\n");
	return dwc2_udc_probe(&s5p6818_otg_data);
}
#endif
