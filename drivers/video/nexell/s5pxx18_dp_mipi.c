/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 *
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <config.h>
#include <common.h>
#include <errno.h>

#include <asm/arch/s5p6818.h>
#include <asm/arch/tieoff.h>
#include <asm/arch/reset.h>
#include <asm/arch/display.h>

#include "soc/s5pxx18_soc_mipi.h"
#include "soc/s5pxx18_soc_disptop.h"
#include "soc/s5pxx18_soc_disptop_clk.h"

#if defined(CONFIG_MACH_S5P6818)
#define	PLLPMS_150MHZ		(0x2192)
#define	BANDCTL_150MHZ		(0x2)
#define	PLLPMS_100MHZ		(0x3323)
#define	BANDCTL_100MHZ		(0x1)
#define	PLLPMS_80MHZ		(0x3283)
#define	BANDCTL_80MHZ		(0x0)
#endif

#define	__io_address(a)	(void *)(uintptr_t)(a)

static void mipi_reset(void)
{
	/* tieoff */
	nx_tieoff_set(NX_TIEOFF_MIPI0_NX_DPSRAM_1R1W_EMAA, 3);
	nx_tieoff_set(NX_TIEOFF_MIPI0_NX_DPSRAM_1R1W_EMAB, 3);

	/* reset */
	nx_rstcon_setrst(RESET_ID_MIPI, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_MIPI_DSI, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_MIPI_CSI, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_MIPI_PHY_S, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_MIPI_PHY_M, RSTCON_ASSERT);

	nx_rstcon_setrst(RESET_ID_MIPI, RSTCON_NEGATE);
	nx_rstcon_setrst(RESET_ID_MIPI_DSI, RSTCON_NEGATE);
	nx_rstcon_setrst(RESET_ID_MIPI_PHY_S, RSTCON_NEGATE);
	nx_rstcon_setrst(RESET_ID_MIPI_PHY_M, RSTCON_NEGATE);
}

static void mipi_init(void)
{
	int clkid = DP_CLOCK_MIPI;
	void *base;

	/*
	 * neet to reset befor open
	 */
	mipi_reset();

	base = __io_address(nx_disp_top_clkgen_get_physical_address(clkid));
	nx_disp_top_clkgen_set_base_address(clkid, base);
	nx_disp_top_clkgen_set_clock_pclk_mode(clkid, nx_pclkmode_always);

	base = __io_address(nx_mipi_get_physical_address(0));
	nx_mipi_set_base_address(0, base);

	nx_mipi_open_module(0);
}

static void mipi_enable(int enable)
{
	int clkid = DP_CLOCK_MIPI;
	int on = (enable ? 1 : 0);

	/* SPDIF and MIPI */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, 1);

	/* START: CLKGEN, MIPI is started in setup function */
	nx_disp_top_clkgen_set_clock_divisor_enable(clkid, on);
	nx_mipi_dsi_set_enable(0, on);
}

static int mipi_setup(int module, int input,
		      struct dp_sync_info *sync, struct dp_ctrl_info *ctrl,
		      struct dp_mipi_dev *dev)
{
	int clkid = DP_CLOCK_MIPI;
	int width = sync->h_active_len;
	int height = sync->v_active_len;
	int index = 0;
	int ret = 0;

	int HFP = sync->h_front_porch;
	int HBP = sync->h_back_porch;
	int HS = sync->h_sync_width;
	int VFP = sync->v_front_porch;
	int VBP = sync->v_back_porch;
	int VS = sync->v_sync_width;

	unsigned int pllpms = dev->pllpms;
	unsigned int bandctl = dev->bandctl;
	unsigned int pllctl = dev->pllctl;
	unsigned int phyctl = dev->phyctl;

#if defined(CONFIG_MACH_S5P6818)
	u32 esc_pre_value = 1;
#else
	u32 esc_pre_value = 10;
#endif

	switch (input) {
	case DP_DEVICE_DP0:
		input = 0;
		break;
	case DP_DEVICE_DP1:
		input = 1;
		break;
	case DP_DEVICE_RESCONV:
		input = 2;
		break;
	default:
		return -EINVAL;
	}

#if defined(CONFIG_MACH_S5P6818)
	nx_mipi_dsi_set_pll(index, 1, 0xFFFFFFFF,
			    PLLPMS_100MHZ, BANDCTL_100MHZ, 0, 0);
#else
	nx_mipi_dsi_set_pll(index, 1, 0xFFFFFFFF,
			    pllpms, bandctl, pllctl, phyctl);
#endif
	mdelay(20);

	if (dev->dev_init) {
		nx_mipi_dsi_software_reset(index);
		nx_mipi_dsi_set_clock(index, 0, 0, 1, 1, 1, 0, 0, 0, 1,
				      esc_pre_value);
		nx_mipi_dsi_set_phy(index, 0, 1, 1, 0, 0, 0, 0, 0);

		/* run callback */
		ret = dev->dev_init(width, height, dev->private_data);
		if (0 > ret)
			return ret;
	}
#if defined(CONFIG_MACH_S5P6818)
	nx_mipi_dsi_set_pll(index, 1, 0xFFFFFFFF,
			    pllpms, bandctl, pllctl, phyctl);
	mdelay(1);

	nx_mipi_dsi_set_clock(index, 0, 0, 1, 1, 1, 0, 0, 0, 1, 10);
	mdelay(1);
#endif

	nx_mipi_dsi_software_reset(index);
	nx_mipi_dsi_set_clock(index, 1, 0, 1, 1, 1, 1, 1, 1, 1, esc_pre_value);
	nx_mipi_dsi_set_phy(index, 3, 1, 1, 1, 1, 1, 0, 0);
	nx_mipi_dsi_set_config_video_mode(index, 1, 0, 1,
					  nx_mipi_dsi_syncmode_event, 1, 1, 1,
					  1, 1, 0, nx_mipi_dsi_format_rgb888,
					  HFP, HBP, HS, VFP, VBP, VS, 0);

	nx_mipi_dsi_set_size(index, width, height);
	nx_disp_top_set_mipimux(1, input);

	/*  0 is spdif, 1 is mipi vclk */
	nx_disp_top_clkgen_set_clock_source(clkid, 1, ctrl->clk_src_lv0);
	nx_disp_top_clkgen_set_clock_divisor(clkid, 1,
					     ctrl->clk_div_lv1 *
					     ctrl->clk_div_lv0);
	return 0;
}

/*
 * disply
 * MIPI DSI Setting
 *		(1) Initiallize MIPI(DSIM,DPHY,PLL)
 *		(2) Initiallize LCD
 *		(3) ReInitiallize MIPI(DSIM only)
 *		(4) Turn on display(MLC,DPC,...)
 */
void nx_mipi_display(int module,
		     struct dp_sync_info *sync, struct dp_ctrl_info *ctrl,
		     struct dp_plane_top *top, struct dp_plane_info *planes,
		     struct dp_mipi_dev *dev)
{
	struct dp_plane_info *plane = planes;
	int input = module == 0 ? DP_DEVICE_DP0 : DP_DEVICE_DP1;
	int count = top->plane_num;
	int i = 0;

	printf("MIPI: dp.%d\n", module);

	dp_control_init(module);
	dp_plane_init(module);

	mipi_init();

	/* set plane */
	dp_plane_screen_setup(module, top);

	for (i = 0; count > i; i++, plane++) {
		if (!plane->enable)
			continue;
		dp_plane_layer_setup(module, plane);
		dp_plane_layer_enable(module, plane, 1);
	}
	dp_plane_screen_enable(module, 1);

	/* set mipi */
	mipi_setup(module, input, sync, ctrl, dev);

	mipi_enable(1);

	/* set dp control */
	dp_control_setup(module, sync, ctrl);
	dp_control_enable(module, 1);
}
