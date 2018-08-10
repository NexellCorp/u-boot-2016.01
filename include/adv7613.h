/*
 * (C) Copyright 2018 Nexell
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#ifndef __ADV7613_H
#define __ADV7613_H

struct adv7613_ops {
	int (*init)(struct udevice *dev);
};

int adv7613_init(struct udevice *dev);
void adv7613_init_s(void);

#endif

