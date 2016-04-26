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

DECLARE_GLOBAL_DATA_PTR;

/*------------------------------------------------------------------------------
 * intialize nexell soc and board status.
 */

void serial_clock_init(void)
{
	char dev[10];
	int id;

	sprintf(dev, "nx-uart.%d", CONFIG_CONS_INDEX);
	id = RESET_ID_UART0 + CONFIG_CONS_INDEX;

	struct clk *clk = clk_get((const char *)dev);

	/* reset control: Low active ___|---   */
	nx_rstcon_setrst(id, RSTCON_ASSERT);
	udelay(10);
	nx_rstcon_setrst(id, RSTCON_NEGATE);
	udelay(10);

	/* set clock   */
	clk_disable(clk);
	clk_set_rate(clk, CONFIG_PL011_CLOCK);
	clk_enable(clk);
}

/* call from u-boot */
int board_early_init_f(void)
{
	serial_clock_init();
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
