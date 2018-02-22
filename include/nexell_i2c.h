/*
 * (C) Copyright 2017 ZhongHong
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#ifndef __NEXELL_I2C_H
#define __NEXELL_I2C_H

struct nx_i2c_ops {
	int (*init)(struct udevice *dev);
	int (*set_mode)(struct udevice *dev, int mode);
};

void nx_i2c_init_s(u8 *reg, u8 *val);
int nx_i2c_set_mode(struct udevice *dev, int mode);
int nx_i2c_init(struct udevice *dev);
#endif

