/*
 * (C) Copyright 2016 Nexell
 * Hyunseok, Jung <hsjung@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/system.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <asm/io.h>
#include <asm/arch/nexell.h>
#include <asm/arch/clk.h>
#include <asm/arch/reset.h>
#include <asm/arch/tieoff.h>
#include <asm/arch/nx_gpio.h>

DECLARE_GLOBAL_DATA_PTR;

#ifndef	CONFIG_ARCH_CPU_INIT
#error must be define the macro "CONFIG_ARCH_CPU_INIT"
#endif

void s_init(void)
{
}

static void cpu_soc_init(void)
{
	/*
	 * NOTE> ALIVE Power Gate must enable for Alive register access.
	 *	     must be clear wfi jump address
	 */
	writel(1, ALIVEPWRGATEREG);
	writel(0xFFFFFFFF, SCR_ARM_SECOND_BOOT);

	/* set l2 cache tieoff */
	nx_tieoff_set(NX_TIEOFF_CORTEXA9MP_TOP_QUADL2C_L2RET1N_0, 1);
	nx_tieoff_set(NX_TIEOFF_CORTEXA9MP_TOP_QUADL2C_L2RET1N_1, 1);
}

#ifdef CONFIG_PL011_SERIAL
static void serial_set_pad_function(int index)
{
	switch (index) {
	case 0:
		/* gpiod-14, gpiod-18 */
		nx_gpio_set_pad_function(3, 14, 1);
		nx_gpio_set_pad_function(3, 18, 1);
		break;
	case 1:
		/* gpiod-15, gpiod-19, gpioc-5, gpioc-6 */
		nx_gpio_set_pad_function(3, 15, 1);
		nx_gpio_set_pad_function(3, 19, 1);
		nx_gpio_set_pad_function(2, 5, 2);
		nx_gpio_set_pad_function(2, 6, 2);
		break;
	case 2:
		/* gpiod-16, gpiod-20 */
		nx_gpio_set_pad_function(3, 16, 1);
		nx_gpio_set_pad_function(3, 20, 1);
		break;
	case 3:
		/* gpiod-17, gpiod-21 */
		nx_gpio_set_pad_function(3, 17, 1);
		nx_gpio_set_pad_function(3, 21, 1);
		break;
	case 4:
		/* gpiob-28, gpiob-29 */
		nx_gpio_set_pad_function(1, 28, 3);
		nx_gpio_set_pad_function(1, 29, 3);
		break;
	case 5:
		/* gpiob-30, gpiob-31 */
		nx_gpio_set_pad_function(1, 30, 3);
		nx_gpio_set_pad_function(1, 31, 3);
		break;
	}
}

static void serial_device_init(void)
{
	char dev[10];
	int id;

	serial_set_pad_function(CONFIG_CONS_INDEX);

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
#endif

int arch_cpu_init(void)
{
	flush_dcache_all();
	cpu_soc_init();
	clk_init();

#ifdef CONFIG_PL011_SERIAL
	serial_device_init();
#endif
	return 0;
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	return 0;
}
#endif

void reset_cpu(ulong ignored)
{
	void *clkpwr_reg = (void *)PHY_BASEADDR_CLKPWR;
	const u32 sw_rst_enb_bitpos = 3;
	const u32 sw_rst_enb_mask = 1 << sw_rst_enb_bitpos;
	const u32 sw_rst_bitpos = 12;
	const u32 sw_rst_mask = 1 << sw_rst_bitpos;
	int pwrcont = 0x224;
	int pwrmode = 0x228;
	u32 read_value;

	read_value = readl((void *)(clkpwr_reg + pwrcont));

	read_value &= ~sw_rst_enb_mask;
	read_value |= 1 << sw_rst_enb_bitpos;

	writel(read_value, (void *)(clkpwr_reg + pwrcont));
	writel(sw_rst_mask, (void *)(clkpwr_reg + pwrmode));
}

void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}

#if defined(CONFIG_ARCH_MISC_INIT)
int arch_misc_init(void)
{
	return 0;
}
#endif	/* CONFIG_ARCH_MISC_INIT */
