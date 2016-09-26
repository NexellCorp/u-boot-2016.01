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
static struct dwc2_plat_otg_data nx_otg_data = {
	.regs_phy = 0xc0011000,
	.regs_otg = 0xc0040000,
};

static void board_udc_probe(void)
{
	dwc2_udc_probe(&nx_otg_data);
}
#endif

int board_init(void)
{
#ifdef CONFIG_SILENT_CONSOLE
	gd->flags |= GD_FLG_SILENT;
#endif

#if !defined CONFIG_OF_CONTROL && !defined CONFIG_DM_USB &&	\
	 defined CONFIG_USB_GADGET
	board_udc_probe();
#endif
	return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
#ifdef CONFIG_SILENT_CONSOLE
	gd->flags &= ~GD_FLG_SILENT;
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

