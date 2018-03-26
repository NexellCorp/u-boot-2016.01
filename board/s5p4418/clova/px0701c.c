/*
 * Copyright (C) 2017  Nexell Co., Ltd.
 *
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <linux/compat.h>
#include <linux/err.h>
#include <asm/gpio.h>
#include <asm/arch/mipi_display.h>

#define PX0701C_WIDTH_MM	94.2
#define PX0701C_HEIGHT_MM	150.72

#define RESET_GPIO		21	/* GPA21 */
#define RESET_DELAY		1	/* ms */
#define POWER_ON_DELAY		1	/* ms */
#define INIT_DELAY		200	/* ms */
#define FLIP_HORIZONTAL		0
#define FLIP_VERTICAL		0

#define mdelay(a)	udelay(a * 1000)

struct px0701c {
	struct mipi_dsi_device *dsi;
	int reset_gpio;
	u32 reset_delay;
	u32 power_on_delay;
	u32 init_delay;
	bool flip_horizontal;
	bool flip_vertical;
	bool is_power_on;
	int error;
};

static inline struct px0701c *dsi_to_px0701c(struct mipi_dsi_device *dsi)
{
	return dsi->ops->private_data;
}

static void _dcs_write(struct px0701c *ctx, const void *data, size_t len)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	int ret;

	if (ctx->error < 0)
		return;

	ret = dsi->write_buffer(dsi, data, len);
	if (ret < 0) {
		printf("error %zd writing dcs seq: %*ph\n", ret,
		       (int)len, data);
		ctx->error = ret;
	}
}

static void __maybe_unused _dcs_read(struct px0701c *ctx, u8 cmd, void *data,
				     size_t len)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	ssize_t ret;

	if (ctx->error < 0)
		return;

	ret = dsi->read_buffer(dsi, cmd, data, len);
	if (ret < 0) {
		printf("error in %s\n", __func__);
		ctx->error = ret;
	}
}

#define _dcs_write_seq(ctx, seq...) \
({\
	const u8 d[] = { seq };\
	BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64, "DCS sequence too big for stack");\
	_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

#define _dcs_write_seq_static(ctx, seq...) \
({\
	static const u8 d[] = { seq };\
	_dcs_write(ctx, d, ARRAY_SIZE(d));\
})


static void _set_sequence(struct px0701c *ctx)
{
	if (ctx->error != 0)
		return;

	//Page0
	_dcs_write_seq_static(ctx,0xE0,0x00);

	//--- PASSWORD  ----//
	_dcs_write_seq_static(ctx,0xE1,0x93);
	_dcs_write_seq_static(ctx,0xE2,0x65);
	_dcs_write_seq_static(ctx,0xE3,0xF8);
	_dcs_write_seq_static(ctx,0x80,0x03);

//--- Sequence Ctrl  ----//

	//--- Page1  ----//
	_dcs_write_seq_static(ctx,0xE0,0x01);

	//Set VCOM
	_dcs_write_seq_static(ctx,0x00,0x00);
	_dcs_write_seq_static(ctx,0x01,0x8E);//A0
	_dcs_write_seq_static(ctx,0x03,0x00);
	_dcs_write_seq_static(ctx,0x04,0x8E);//A0

	//Set Gamma Power,1, VGMP,1,VGMN,1,VGSP,1,VGSN
	//_dcs_write_seq_static(ctx,0x0A,1,0x07);
	_dcs_write_seq_static(ctx,0x0C,0x74);

	//Set Gamma Power,1, VGMP,1,VGMN,1,VGSP,1,VGSN
	_dcs_write_seq_static(ctx,0x17,0x00);
	_dcs_write_seq_static(ctx,0x18,0xD8);//D7
	_dcs_write_seq_static(ctx,0x19,0x01);//00
	_dcs_write_seq_static(ctx,0x1A,0x00);
	_dcs_write_seq_static(ctx,0x1B,0xD8);
	_dcs_write_seq_static(ctx,0x1C,0x01);//00

	//Set Gate Power
	_dcs_write_seq_static(ctx,0x1F,0x74);	//VGH_REG=17V	6A VGH_REG=15V
	_dcs_write_seq_static(ctx,0x20,0x19);	//VGL_REG=-8V	23 VGL_REG=-10V
	_dcs_write_seq_static(ctx,0x21,0x19);	//VGL_REG2=-8V	23 VGL_REG2=-10V
	_dcs_write_seq_static(ctx,0x22,0x0E);

	//SET RGBCYC
	_dcs_write_seq_static(ctx,0x37,0x29);	//SS=1,1, BGR=1
	_dcs_write_seq_static(ctx,0x38,0x05);	//JDT=101 Zig-zag
	_dcs_write_seq_static(ctx,0x39,0x08);	//RGB_N_EQ1,1, modify 20140806
	_dcs_write_seq_static(ctx,0x3A,0x18);	//12	modify 15/05/06	RGB_N_EQ2,1, modify 20140806
	_dcs_write_seq_static(ctx,0x3B,0x18);	//modify 15/05/06
	_dcs_write_seq_static(ctx,0x3C,0x72);	//78	modify 15/05/06	SET EQ3 for TE_H
	_dcs_write_seq_static(ctx,0x3D,0xFF);	//SET CHGEN_ON,1, modify 20140827
	_dcs_write_seq_static(ctx,0x3E,0xFF);	//SET CHGEN_OFF,1, modify 20140827
	_dcs_write_seq_static(ctx,0x3F,0xFF);	//SET CHGEN_OFF2,1, modify 20140827

	//Set TCON
	_dcs_write_seq_static(ctx,0x40,0x06);	//RSO 04h=720,1, 05h=768,1, 06h=800
	_dcs_write_seq_static(ctx,0x41,0xA0);	//LN=640->1280 line
	_dcs_write_seq_static(ctx,0x43,0x10);	//VFP
	_dcs_write_seq_static(ctx,0x44,0x0E);	//VBP
	_dcs_write_seq_static(ctx,0x45,0x3C);	//HBP

	//--- power voltage  ----//
	_dcs_write_seq_static(ctx,0x55,0x01);	//DCDCM=0011,1, HX PWR_IC
	_dcs_write_seq_static(ctx,0x56,0x01);
	_dcs_write_seq_static(ctx,0x57,0x64);	//65	modify 15/05/06
	_dcs_write_seq_static(ctx,0x58,0x0A);	//AVDD
	_dcs_write_seq_static(ctx,0x59,0x0A);	//VCL &AVEE
	_dcs_write_seq_static(ctx,0x5A,0x28);	//VGH ,1,15V
	_dcs_write_seq_static(ctx,0x5B,0x10);	//VGL,1,-10V

	//--- Gamma  ----//
	_dcs_write_seq_static(ctx,0x5D,0x7C);	//70
	_dcs_write_seq_static(ctx,0x5E,0x60);  //56
	_dcs_write_seq_static(ctx,0x5F,0x4E);	//45
	_dcs_write_seq_static(ctx,0x60,0x40);	//39
	_dcs_write_seq_static(ctx,0x61,0x39);  //35
	_dcs_write_seq_static(ctx,0x62,0x28);  //26
	_dcs_write_seq_static(ctx,0x63,0x2A);  //2A
	_dcs_write_seq_static(ctx,0x64,0x11);  //14
	_dcs_write_seq_static(ctx,0x65,0x27);  //2E
	_dcs_write_seq_static(ctx,0x66,0x23);  //2D
	_dcs_write_seq_static(ctx,0x67,0x21);  //2D
	_dcs_write_seq_static(ctx,0x68,0x3D);  //4C
	_dcs_write_seq_static(ctx,0x69,0x2B);  //3A
	_dcs_write_seq_static(ctx,0x6A,0x33);  //45
	_dcs_write_seq_static(ctx,0x6B,0x26);  //38
	_dcs_write_seq_static(ctx,0x6C,0x24);  //36
	_dcs_write_seq_static(ctx,0x6D,0x18);  //2B
	_dcs_write_seq_static(ctx,0x6E,0x0A);  //1C
	_dcs_write_seq_static(ctx,0x6F,0x00);  //00

	_dcs_write_seq_static(ctx,0x70,0x7C);  //70
	_dcs_write_seq_static(ctx,0x71,0x60);  //56
	_dcs_write_seq_static(ctx,0x72,0x4E);  //45
	_dcs_write_seq_static(ctx,0x73,0x40);  //39
	_dcs_write_seq_static(ctx,0x74,0x39);  //35
	_dcs_write_seq_static(ctx,0x75,0x29);  //26
	_dcs_write_seq_static(ctx,0x76,0x2B);  //2A
	_dcs_write_seq_static(ctx,0x77,0x12);  //14
	_dcs_write_seq_static(ctx,0x78,0x27);  //2E
	_dcs_write_seq_static(ctx,0x79,0x24);  //2D
	_dcs_write_seq_static(ctx,0x7A,0x21);  //2D
	_dcs_write_seq_static(ctx,0x7B,0x3E);  //4C
	_dcs_write_seq_static(ctx,0x7C,0x2B);  //3A
	_dcs_write_seq_static(ctx,0x7D,0x33);  //45
	_dcs_write_seq_static(ctx,0x7E,0x26);  //38
	_dcs_write_seq_static(ctx,0x7F,0x24);  //36
	_dcs_write_seq_static(ctx,0x80,0x19);  //2B
	_dcs_write_seq_static(ctx,0x81,0x0A);  //1C
	_dcs_write_seq_static(ctx,0x82,0x00);  //00


	//Page2,1, for GIP
	_dcs_write_seq_static(ctx,0xE0,0x02);
	//GIP_L Pin mapping
	_dcs_write_seq_static(ctx,0x00,0x0E);	//L1/CK2BO/CKV10
	_dcs_write_seq_static(ctx,0x01,0x06);  //L2/CK2O/CKV2
	_dcs_write_seq_static(ctx,0x02,0x0C);  //L3/CK1BO/CKV8
	_dcs_write_seq_static(ctx,0x03,0x04);  //L4/CK1O/CKV0
	_dcs_write_seq_static(ctx,0x04,0x08);  //L5/CK3O/CKV4
	_dcs_write_seq_static(ctx,0x05,0x19);  //L6/CK3BO/CKV2
	_dcs_write_seq_static(ctx,0x06,0x0A);  //L7/CK4O/CKV6
	_dcs_write_seq_static(ctx,0x07,0x1B);  //L8/CK4BO/CKV14
	_dcs_write_seq_static(ctx,0x08,0x00);  //L9/STVO/STV0
	_dcs_write_seq_static(ctx,0x09,0x1D);  //L10
	_dcs_write_seq_static(ctx,0x0A,0x1F);  //L11/VGL
	_dcs_write_seq_static(ctx,0x0B,0x1F);  //L12/VGL
	_dcs_write_seq_static(ctx,0x0C,0x1D);  //L13
	_dcs_write_seq_static(ctx,0x0D,0x1D);  //L14
	_dcs_write_seq_static(ctx,0x0E,0x1D);  //L15
	_dcs_write_seq_static(ctx,0x0F,0x17);  //L16/V1_O/FLM1
	_dcs_write_seq_static(ctx,0x10,0x37);  //L17/V2_O/FLM1_INV
	_dcs_write_seq_static(ctx,0x11,0x1D);  //L18
	_dcs_write_seq_static(ctx,0x12,0x1F);  //L19/BW/VGL
	_dcs_write_seq_static(ctx,0x13,0x1E);  //L20/FW/VGH
	_dcs_write_seq_static(ctx,0x14,0x10);  //L21/RST_O/ETV0
	_dcs_write_seq_static(ctx,0x15,0x1D);  //L22

	//GIP_R Pin mapping
	_dcs_write_seq_static(ctx,0x16,0x0F);  //R1/CK2BE/CKV11
	_dcs_write_seq_static(ctx,0x17,0x07);  //R2/CK2E/CKV3
	_dcs_write_seq_static(ctx,0x18,0x0D);  //R3/CK1BE/CKV9
	_dcs_write_seq_static(ctx,0x19,0x05);  //R4/CK1E/CKV1
	_dcs_write_seq_static(ctx,0x1A,0x09);  //R5/CK3E/CKV5
	_dcs_write_seq_static(ctx,0x1B,0x1A);  //R6/CK3BE/CKV13
	_dcs_write_seq_static(ctx,0x1C,0x0B);  //R7/CK4E/CKV7
	_dcs_write_seq_static(ctx,0x1D,0x1C);  //R8/CK4BE/CKV15
	_dcs_write_seq_static(ctx,0x1E,0x01);  //R9/STVE/STV1
	_dcs_write_seq_static(ctx,0x1F,0x1D);  //R10
	_dcs_write_seq_static(ctx,0x20,0x1F);  //R11/VGL
	_dcs_write_seq_static(ctx,0x21,0x1F);  //R12/VGL
	_dcs_write_seq_static(ctx,0x22,0x1D);  //R13
	_dcs_write_seq_static(ctx,0x23,0x1D);  //R14
	_dcs_write_seq_static(ctx,0x24,0x1D);  //R15
	_dcs_write_seq_static(ctx,0x25,0x18);  //R16/V1_E/FLM2
	_dcs_write_seq_static(ctx,0x26,0x38);  //R17/V2_E/FLM2_INV
	_dcs_write_seq_static(ctx,0x27,0x1D);  //R18
	_dcs_write_seq_static(ctx,0x28,0x1F);  //R19/BW/VGL
	_dcs_write_seq_static(ctx,0x29,0x1E);  //R20/FW/VGH
	_dcs_write_seq_static(ctx,0x2A,0x11);  //R21/RST_E/ETV1
	_dcs_write_seq_static(ctx,0x2B,0x1D);  //R22

	//GIP_L_GS Pin mapping
	_dcs_write_seq_static(ctx,0x2C,0x09);  //L1/CK2BO/CKV5
	_dcs_write_seq_static(ctx,0x2D,0x1A);  //L2/CK2O/CKV13
	_dcs_write_seq_static(ctx,0x2E,0x0B);  //L3/CK1BO/CKV7
	_dcs_write_seq_static(ctx,0x2F,0x1C);  //L4/CK1O/CKV15
	_dcs_write_seq_static(ctx,0x30,0x0F);  //L5/CK3O/CKV11
	_dcs_write_seq_static(ctx,0x31,0x07);  //L6/CK3BO/CKV3
	_dcs_write_seq_static(ctx,0x32,0x0D);  //L7/CK4O/CKV9
	_dcs_write_seq_static(ctx,0x33,0x05);  //L8/CK4BO/CKV1
	_dcs_write_seq_static(ctx,0x34,0x11);  //L9/STVO/ETV1
	_dcs_write_seq_static(ctx,0x35,0x1D);  //L10
	_dcs_write_seq_static(ctx,0x36,0x1F);  //L11/VGL
	_dcs_write_seq_static(ctx,0x37,0x1F);  //L12/VGL
	_dcs_write_seq_static(ctx,0x38,0x1D);  //L13
	_dcs_write_seq_static(ctx,0x39,0x1D);  //L14
	_dcs_write_seq_static(ctx,0x3A,0x1D);  //L15
	_dcs_write_seq_static(ctx,0x3B,0x18);  //L16/V1_O/FLM2
	_dcs_write_seq_static(ctx,0x3C,0x38);  //L17/V2_O/?
	_dcs_write_seq_static(ctx,0x3D,0x1D);  //L18
	_dcs_write_seq_static(ctx,0x3E,0x1E);  //L19/BW/VGH
	_dcs_write_seq_static(ctx,0x3F,0x1F);  //L20/FW/VGL
	_dcs_write_seq_static(ctx,0x40,0x01);  //L21/RST_O/STV1
	_dcs_write_seq_static(ctx,0x41,0x1D);  //L22

	//GIP_R_GS Pin mapping
	_dcs_write_seq_static(ctx,0x42,0x08);  //R1/CK2BE/CKV4
	_dcs_write_seq_static(ctx,0x43,0x19);  //R2/CK2E/CKV12
	_dcs_write_seq_static(ctx,0x44,0x0A);  //R3/CK1BE/CKV6
	_dcs_write_seq_static(ctx,0x45,0x1B);  //R4/CK1E/CKV14
	_dcs_write_seq_static(ctx,0x46,0x0E);  //R5/CK3E/CKV10
	_dcs_write_seq_static(ctx,0x47,0x06);  //R6/CK3BE/CKV2
	_dcs_write_seq_static(ctx,0x48,0x0C);  //R7/CK4E/CKV8
	_dcs_write_seq_static(ctx,0x49,0x04);  //R8/CK4BE/CKV0
	_dcs_write_seq_static(ctx,0x4A,0x10);  //R9/STVE/ETV0
	_dcs_write_seq_static(ctx,0x4B,0x1D);  //R10
	_dcs_write_seq_static(ctx,0x4C,0x1F);  //R11/VGL
	_dcs_write_seq_static(ctx,0x4D,0x1F);  //R12/VGL
	_dcs_write_seq_static(ctx,0x4E,0x1D);  //R13
	_dcs_write_seq_static(ctx,0x4F,0x1D);  //R14
	_dcs_write_seq_static(ctx,0x50,0x1D);  //R15
	_dcs_write_seq_static(ctx,0x51,0x17);  //R16/V1_E/FLM1
	_dcs_write_seq_static(ctx,0x52,0x37);  //R17/V2_E/?
	_dcs_write_seq_static(ctx,0x53,0x1D);  //R18
	_dcs_write_seq_static(ctx,0x54,0x1E);  //R19/BW/VGH
	_dcs_write_seq_static(ctx,0x55,0x1F);  //R20/FW/VGL
	_dcs_write_seq_static(ctx,0x56,0x00);  //R21/RST_E/STV0
	_dcs_write_seq_static(ctx,0x57,0x1D);  //R22

	//GIP Timing
	_dcs_write_seq_static(ctx,0x58,0x10);
	_dcs_write_seq_static(ctx,0x59,0x00);
	_dcs_write_seq_static(ctx,0x5A,0x00);
	_dcs_write_seq_static(ctx,0x5B,0x10);
	_dcs_write_seq_static(ctx,0x5C,0x00);  //01
	_dcs_write_seq_static(ctx,0x5D,0xD0);  //50
	_dcs_write_seq_static(ctx,0x5E,0x01);
	_dcs_write_seq_static(ctx,0x5F,0x02);
	_dcs_write_seq_static(ctx,0x60,0x60);
	_dcs_write_seq_static(ctx,0x61,0x01);
	_dcs_write_seq_static(ctx,0x62,0x02);
	_dcs_write_seq_static(ctx,0x63,0x06);
	_dcs_write_seq_static(ctx,0x64,0x6A);
	_dcs_write_seq_static(ctx,0x65,0x55);
	_dcs_write_seq_static(ctx,0x66,0x0F);	  //2C
	_dcs_write_seq_static(ctx,0x67,0xF7);  //73
	_dcs_write_seq_static(ctx,0x68,0x08);  //05
	_dcs_write_seq_static(ctx,0x69,0x08);
	_dcs_write_seq_static(ctx,0x6A,0x6A);  //66_by Max_20151029
	_dcs_write_seq_static(ctx,0x6B,0x10);	//dummy clk
	_dcs_write_seq_static(ctx,0x6C,0x00);
	_dcs_write_seq_static(ctx,0x6D,0x00);
	_dcs_write_seq_static(ctx,0x6E,0x00);
	_dcs_write_seq_static(ctx,0x6F,0x88);
	_dcs_write_seq_static(ctx,0x70,0x00);  //00
	_dcs_write_seq_static(ctx,0x71,0x17);  //00
	_dcs_write_seq_static(ctx,0x72,0x06);
	_dcs_write_seq_static(ctx,0x73,0x7B);
	_dcs_write_seq_static(ctx,0x74,0x00);
	_dcs_write_seq_static(ctx,0x75,0x80);  //80
	_dcs_write_seq_static(ctx,0x76,0x01);
	_dcs_write_seq_static(ctx,0x77,0x5D);  //0D
	_dcs_write_seq_static(ctx,0x78,0x18);  //18
	_dcs_write_seq_static(ctx,0x79,0x00);
	_dcs_write_seq_static(ctx,0x7A,0x00);
	_dcs_write_seq_static(ctx,0x7B,0x00);
	_dcs_write_seq_static(ctx,0x7C,0x00);
	_dcs_write_seq_static(ctx,0x7D,0x03);
	_dcs_write_seq_static(ctx,0x7E,0x7B);

	//Page4
	_dcs_write_seq_static(ctx,0xE0,0x04);
	//_dcs_write_seq_static(ctx,0x04,0x01);	//00 modify 15/05/06
	_dcs_write_seq_static(ctx,0x09,0x10);	// modify 15/05/06
	_dcs_write_seq_static(ctx,0x0E,0x38);	// modify 15/05/06

	//ESD Check & lane number
	_dcs_write_seq_static(ctx,0x2B,0x2B);
	_dcs_write_seq_static(ctx,0x2D,0x03);
	_dcs_write_seq_static(ctx,0x2E,0x44);

	//Page0
	_dcs_write_seq_static(ctx,0xE0,0x00);

	//Watch dog
	_dcs_write_seq_static(ctx,0xE6,0x02);
	_dcs_write_seq_static(ctx,0xE7,0x06);
	//_dcs_write_seq_static(ctx,0x11,1,0x0);
	_dcs_write_seq_static(ctx,0x11);
	mdelay(200);

	//_dcs_write_seq_static(ctx,0x29,1,0x0);
	_dcs_write_seq_static(ctx,0x29);
	mdelay(20);
	/* all pixel on */
	//_dcs_write_seq_static(ctx, 0x23);
}

static int px0701c_power_on(struct px0701c *ctx)
{
	if (ctx->is_power_on)
		return 0;

	mdelay(ctx->power_on_delay);

	gpio_direction_output(ctx->reset_gpio, 1);
#if 0
	int count = 100;
	while (count > 0) {
		gpio_set_value(ctx->reset_gpio, 0);
		mdelay(1);
		gpio_set_value(ctx->reset_gpio, 1);
		mdelay(5);
		count--;
	}
#else
	mdelay(10);
	gpio_set_value(ctx->reset_gpio, 0);
	//mdelay(ctx->reset_delay);
	mdelay(20);
	/* mdelay(10); */
	gpio_set_value(ctx->reset_gpio, 1);
	/* mdelay(5); */
	//mdelay(ctx->power_on_delay);
	mdelay(120);
#endif

	ctx->is_power_on = true;

	return 0;
}

static int px0701c_power_off(struct px0701c *ctx)
{
	if (!ctx->is_power_on)
		return 0;

	gpio_set_value(ctx->reset_gpio, 0);
	ctx->is_power_on = false;

	return 0;
}

static int px0701c_prepare(struct mipi_dsi_device *dsi)
{
	struct px0701c *ctx = dsi_to_px0701c(dsi);
	int ret;


	ret = px0701c_power_on(ctx);
	if (ret < 0)
		return ret;

	_set_sequence(ctx);
#if 0
    /* sleep out */
    _dcs_write_seq_static(ctx, 0x11);
    mdelay(120);
    /* printf("%s: %d\n", __func__, __LINE__); */

    /* display on */
    _dcs_write_seq_static(ctx, 0x29);
    mdelay(5);
#endif

	ret = ctx->error;

	if (ret < 0)
		return px0701c_power_off(ctx);

	return ret;
}

static int px0701c_unprepare(struct mipi_dsi_device *dsi)
{
	struct px0701c *ctx = dsi_to_px0701c(dsi);

	return px0701c_power_off(ctx);
}

static int px0701c_enable(struct mipi_dsi_device *dsi)
{
	return 0;
}

static int px0701c_disable(struct mipi_dsi_device *dsi)
{
	return 0;
}

static struct mipi_panel_ops px0701c_ops = {
	.prepare = px0701c_prepare,
	.unprepare = px0701c_unprepare,
	.enable = px0701c_enable,
	.disable = px0701c_disable,
};

static int px0701c_parse_dt(struct px0701c *ctx)
{
	ctx->reset_gpio = RESET_GPIO;
	ctx->reset_delay = RESET_DELAY;
	ctx->power_on_delay = POWER_ON_DELAY;
	ctx->init_delay = INIT_DELAY;
	ctx->flip_horizontal = FLIP_HORIZONTAL;
	ctx->flip_vertical = FLIP_VERTICAL;

	return 0;
}

static int px0701c_init(struct mipi_dsi_device *dsi)
{
	struct px0701c *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->dsi = dsi;
	ctx->is_power_on = false;

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO |
		MIPI_DSI_MODE_VIDEO_HFP | MIPI_DSI_MODE_VIDEO_HBP |
		MIPI_DSI_MODE_VIDEO_HSA | MIPI_DSI_MODE_VSYNC_FLUSH;
	dsi->mode_flags |= MIPI_DSI_MODE_LPM;
#if 0
	dsi->mode_flags |= MIPI_DSI_MODE_VIDEO_HSE; // no error
	dsi->mode_flags |= MIPI_DSI_MODE_VIDEO_SYNC_PULSE; // no error
	dsi->mode_flags |= MIPI_DSI_MODE_EOT_PACKET;
#endif
	dsi->ops = &px0701c_ops;
	dsi->ops->private_data = ctx;

	ret = px0701c_parse_dt(ctx);
	if (ret < 0)
		return ret;

	ret = gpio_request(ctx->reset_gpio, "reset-gpio");
	if (ret) {
		printf("fail: px071x request reset-gpio of %d\n", ctx->reset_gpio);
		return ret;
	}

	return 0;
}

int nx_mipi_dsi_lcd_bind(struct mipi_dsi_device *dsi)
{
	return px0701c_init(dsi);
}
