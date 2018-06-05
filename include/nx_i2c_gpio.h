/*
 * (C) Copyright 2018 Nexell
 *
 * Hyejung, Kwon <cjscld15@nexell.co.kr>
 *
 * * SPDX-License-Identifier:      GPL-2.0+
 */

#ifndef __NX_I2C_GPIO_H
#define __NX_I2C_GPIO_H

struct udevice* nx_i2c_gpio_init(void);
int nx_i2c_gpio_write(struct udevice *dev,
		unsigned int cmd, unsigned char *buf, int len);
int nx_i2c_gpio_read(struct udevice *dev,
		     unsigned int cmd, unsigned char *buf, int len);
#endif

