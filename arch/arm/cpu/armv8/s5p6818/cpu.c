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
#include <asm/armv8/mmu.h>
#include <asm/arch/nexell.h>
#include <asm/arch/clk.h>

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region nx_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0x40000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE
	}, {
		.virt = 0x40000000UL,
		.phys = 0x40000000UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0xc0000000UL,
		.phys = 0xc0000000UL,
		.size = 0x20000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE
	}, {
		/* List terminator */
		0,
	}
};
struct mm_region *mem_map = nx_mem_map;

#ifndef	CONFIG_ARCH_CPU_INIT
#error must be define the macro "CONFIG_ARCH_CPU_INIT"
#endif

void cpu_base_init(void)
{
#if defined(CONFIG_S5P6818_WATCHDOG)
    	void *wdt_reg = (void*)PHY_BASEADDR_WDT;
#endif

	/*
	 * NOTE> ALIVE Power Gate must enable for Alive register access.
	 *	     must be clear wfi jump address
	 */
	writel(1, ALIVEPWRGATEREG);
	writel(0xFFFFFFFF, SCR_ARM_SECOND_BOOT);

	/* write 0xf0 on alive scratchpad reg for boot success check */
	writel(readl(SCR_SIGNAGURE_READ) | 0xF0, (SCR_SIGNAGURE_SET));

#if defined(CONFIG_S5P6818_WATCHDOG)
	/* Kick watchdog from earlier boot stage */
	writel(0xffff, wdt_reg + 0x8);
	writel(0x0, wdt_reg + 0xc);
#endif
}

#if defined(CONFIG_ARCH_CPU_INIT)
int arch_cpu_init(void)
{
	flush_dcache_all();
	cpu_base_init();
	clk_init();
	return 0;
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
#if defined(CONFIG_S5P6818_WATCHDOG)
	void *clkpwr_reg = (void *)PHY_BASEADDR_CLKPWR;
	int resetstatus = 0x218;
	u32 read_value;

	read_value = readl((void *)(clkpwr_reg + resetstatus));

	printf("Reset : %x\n", read_value);
#endif

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

#if defined(CONFIG_S5P6818_WATCHDOG)
void watchdog_reset()
{
    	void *wdt_reg = (void*)PHY_BASEADDR_WDT;

	/* Disable watchdog */
	writel(0x0, wdt_reg);
	writel(0x0, wdt_reg + 0xc);
}
#endif

#if defined(CONFIG_ARCH_MISC_INIT)
int arch_misc_init(void)
{
	return 0;
}
#endif	/* CONFIG_ARCH_MISC_INIT */
