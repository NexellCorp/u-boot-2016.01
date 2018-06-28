/*
 * (C) Copyright 2018 Nexell
 * Jinyong, Lee <justin@nexell.co.kr>
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

#ifdef CONFIG_SILENT_CONSOLE
	gd->flags &= ~GD_FLG_SILENT;
#endif

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
