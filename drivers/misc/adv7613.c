/*
 * (C) Copyright 2018 Nexell
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <i2c.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <adv7613.h>

#define RETRY_CNT	3
#define DELAY_TIME	0xFE
#define END_VAL		0xFF

struct adv7613_reg_val {
	unsigned char addr;
	unsigned int cmd;
	unsigned char data;
};

/* 1920x720p@60 HDMI Input_LVDS Output Port A and B: */
static struct adv7613_reg_val adv7613_reg[] = {
	{0x98, 0xFF, 0x80},	//default 0x00, I2C reset
	{DELAY_TIME, 5, 0x00},	//add 5ms delay
	{0x98, 0xF4, 0x80},	//default 0x00, CEC Map Address set to 0x80
	{0x98, 0xF5, 0x7C},	//default 0x00, Infoframe Map Address set to 0x7C
	{0x98, 0xF8, 0x4C},	//default 0x00, DPLL Map Address set to 0x4C
	{0x98, 0xF9, 0x64},	//default 0x00, KSV Map Address set to 0x64
	{0x98, 0xFA, 0x6C},	//default 0x00, EDID Map Address set to 0x6C
	{0x98, 0xFB, 0x68},	//default 0x00, HDMI Map Address set to 0x68
	{0x98, 0xFD, 0x44},	//default 0x00, CP Map Address set to 0x44
	{0x98, 0xE9, 0xC0},	//default 0x00, LVDS Map Address set to 0xC0
	{0x98, 0x00, 0x12},	//default 0x08, [5:0] - VID_STD[5:0] = 6'b010010 - WXGA 1360x768p@60Hz
	{0x98, 0x01, 0x06},	//default 0x06, [3:0] - Prim_Mode[3:0] = 4'b0110 - HDMI-Gr
	{0x98, 0x02, 0xF2},	//default 0xF0, [7:4] - INP_COLOR_SPACE[3:0] = 4'b1111 - color space determined by HDMI block, [1] RGB_OUT - 1'b1 - RGB color space output
	{0x98, 0x03, 0x42},	//default 0x00, [7:0] - OP_FORMAT_SEL[7:0] = 8'b01000010 - 36-bit 4:4:4 SDR Mode 0
	{0x98, 0x04, 0x63},	//enable CP timing adjustment
	{0x98, 0x05, 0x20},	//default 0x2C  [2] - AVCODE_INSERT_EN = 1'b0//added 0x20 disable data blanking for script reload
	{0x98, 0x0C, 0x42},	//default 0x62, [5] - POWER_DOWN = 1'b0 - Powers up part
	{0x98, 0x15, 0xAE},	//default 0xBE, [4] - TRI_AUDIO = 1'b0 = untristate Audio , {3] - TRI_LLC = 1'b1 = tristate LLC, Bit{1] - TRI_PIX = 1'b1 = Tristate Pixel Pads
	{0x44, 0x6C, 0x00},	//default 0x10, ADI Recommended write
	{0x44, 0x8B, 0x40},	//HS DE adjustment
	{0x44, 0x8C, 0x02},	//HS DE adjustment
	{0x44, 0x8D, 0x02},	//HS DE adjustment
	{0x64, 0x40, 0x81},	//default 0x83, BCAPS[7:0] - Disable HDCP 1.1 features
	{0x68, 0x03, 0x98},	//default 0x18, ADI Recommended write
	{0x68, 0x10, 0xA5},	//default 0x25, ADI Recommended write
	{0x68, 0x1B, 0x08},	//default 0x18, ADI Recommended write
	{0x68, 0x45, 0x04},	//default 0x00, ADI Recommended write
	{0x68, 0x97, 0xC0},	//default 0x80, ADI Recommended write
	{0x68, 0x3D, 0x10},	//default 0x00, ADI Recommended write
	{0x68, 0x3E, 0x7B},	//default 0x79, ADI recommended write
	{0x68, 0x3F, 0x5E},	//default 0x63, ADI Recommended Write
	{0x68, 0x4E, 0xFE},	//default 0x7B, ADI recommended write
	{0x68, 0x4F, 0x08},	//default 0x63, ADI recommended write
	{0x68, 0x57, 0xA3},	//default 0x30, ADI recommended write
	{0x68, 0x58, 0x07},	//default 0x01, ADI recommended write
	{0x68, 0x6F, 0x08},	//default 0x00, ADI Recommended write
	{0x68, 0x83, 0xFE},	//default 0xFF, ADI recommended write
	{0x68, 0x86, 0x9B},	//default 0x00, ADI recommended write
	{0x68, 0x85, 0x10},	//default 0x16, ADI recommended write
	{0x68, 0x89, 0x01},	//default 0x00, ADI recommended write
	{0x68, 0x9B, 0x03},	//default 0x0B, ADI Recommended write
	{0x68, 0x9C, 0x80},	//default 0x08, ADI Recommended write
	{0x68, 0x9C, 0xC0},	//default 0x08, ADI Recommended write
	{0x68, 0x9C, 0x00},	//default 0x08, ADI Recommended write
	{DELAY_TIME, 200, 0x00},		//add 200ms delay
	{0xC0, 0x40, 0x08},	//default 0x02, Bit [0] tx_mode_itu656 - 1'b0 = OLDI mode, Bit [1] tx_pdn - 1'b0 = LVDS TX powered on, Bit [3] tx_pll_en - 1'b1 =  power up LVDS PLL
	{0xC0, 0x43, 0x03},	//default 0x00, ADI Recommended write
	{0xC0, 0x44, 0x00},	//default 0x00, PLL GEAR =< 100MHz
	{0xC0, 0x45, 0x04},	//default 0x1E, ADI Recommended write
	{0xC0, 0x46, 0x53},	//default 0x77, ADI Recommended write
	{0xC0, 0x47, 0x03},	//default 0x02, ADI Recommended write
	{0xC0, 0x4C, 0x19},	//default 0x71, Bit [6] tx_oldi_hs_pol - 1'b0 = HS Polarity Neg, Bit [5] tx_oldi_vs_pol - 1'b0 = VS Polarity Neg, Bit [4] tx_oldi_de_pol - 1'b1 =  DE Polarity Pos, Bit [3] tx_enable_ns_mapping - 1'b0 = normal oldi 8-bit mapping, Bit [2] tx_656_all_lanes_enable - 1'b0 = disable 656 data on lanes ,Bit [1] tx_oldi_balanced_mode - 1'b0 = Non DC balanced ,Bit [0] tx_color_mode - 1'b1 = 8-bit mode
	{0xC0, 0x4E, 0x14},	//default 0x08, Bit [3:2] tx_color_depth[1:0] - 2'b01 = 8-bit Mode,Bit [1] tx_int_res - 1'b0 = Res bit 0,Bit [0] tx_mux_int_res - 1'b0 = disable programming of res bit
	{END_VAL, 0x00, 0x00},	//end
};

static int adv7613_i2c_read(struct udevice *dev,
	unsigned char slave_addr, unsigned int cmd,
	unsigned char *buf, int len)
{
	struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);

	chip->chip_addr = slave_addr;

	return dm_i2c_read(dev, cmd, buf, len);
}

static int adv7613_i2c_write(struct udevice *dev,
	unsigned char slave_addr, unsigned int cmd,
	unsigned char *buf, int len)
{
	struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);

	chip->chip_addr = slave_addr;
	chip->flags = DM_I2C_CHIP_WR_ADDRESS;

	return dm_i2c_write(dev, cmd, buf, len);
}

int adv7613_init(struct udevice *dev)
{
	int i;
	int ret = 0;
	struct adv7613_reg_val *reg_val = adv7613_reg;

	while (reg_val->addr != END_VAL) {
		if (reg_val->addr == DELAY_TIME) {
			mdelay(reg_val->cmd);
		} else {
			for (i=0; i<RETRY_CNT; i++) {
				ret = adv7613_i2c_write(dev, reg_val->addr>>1,
					reg_val->cmd, &reg_val->data, 1);
				if (ret >= 0)
					break;
			}
		}
		reg_val++;
	}

	return 0;
}

static int adv7613_probe(struct udevice *dev)
{
	return 0;
}

static int adv7613_ofdata_to_platdata(struct udevice *dev)
{
	return 0;
}

static int adv7613_bind(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id adv7613_ids[] = {
	{ .compatible = "ad,adv7613" },
	{}
};

static struct adv7613_ops this_ops = {
	.init = adv7613_init,
};

U_BOOT_DRIVER(adv7613) = {
	.name = "adv7613",
	.id = UCLASS_ADV7613_ID,
	.of_match = adv7613_ids,
	.probe = adv7613_probe,
	.bind = adv7613_bind,
	.ops = &this_ops,
};

void adv7613_init_s(void)
{
	struct udevice *dev;
	int ret;
	const struct adv7613_ops *ops;

	ret = uclass_get_device(UCLASS_ADV7613_ID, 0, &dev);
	if (ret)
		return;

	ops = device_get_ops(dev);
	if (ops->init)
		ret = ops->init(dev);
}

UCLASS_DRIVER(adv7613) = {
	.id = UCLASS_ADV7613_ID,
	.name = "adv7613",
};

