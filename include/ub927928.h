/*
 * (C) Copyright 2017 ZhongHong
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#ifndef __UB927928_H
#define __UB927928_H

struct ub927928_ops {
	int (*init)(struct udevice *dev);
	int (*set_mode)(struct udevice *dev, int mode);
};

int ub927928_init(struct udevice *dev);
int ub927928_set_mode(struct udevice *dev, int mode);
void ub927928_init_s(void);
#endif

