/*
 * Pinctrl driver for Nexell SoCs
 * (C) Copyright 2017 Nexell
 * Jongshin, Park <pjsin865@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fdtdec.h>
#include <errno.h>
#include <dm.h>
#include <i2c.h>
#include <misc.h>

#include <linux/time.h>
#include <linux/list.h>
#include <asm/io.h>

struct i2c_bus {
	int		id;
	int		speed;
	ulong	data;
};

static int ds90ub92x_i2c_set_bus_speed(struct udevice *dev, unsigned int speed)
{
	return 0;
}

static int ds90ub92x_i2c_xfer(struct udevice *dev, struct i2c_msg *msg,
			    int nmsgs)
{
	return 0;
}

static int ds90ub92x_ofdata_to_platdata(struct udevice *dev)
{
	return 0;
}

static int ds90ub92x_probe(struct udevice *dev)
{
	struct i2c_bus *i2c_bus = dev_get_priv(dev);

	i2c_bus->id = dev->seq;
	i2c_bus->data = dev_get_driver_data(dev);

	return 0;
}

static int ds90ub92x_bind(struct udevice *dev)
{
	return 0;
}


static const struct dm_i2c_ops ds90ub92x_i2c_ops = {
	.xfer		= ds90ub92x_i2c_xfer,
	.set_bus_speed	= ds90ub92x_i2c_set_bus_speed,
};

static const struct udevice_id ds90ub92x_ids[] = {
	{ .compatible = "ti,ds90ub927", .data = (ulong)1},
	{ .compatible = "ti,ds90ub928", .data = (ulong)2},
	{}
};

U_BOOT_DRIVER(ds90ub92x) = {
	.name	= "ds90ub92x",
	.id	= UCLASS_I2C,
	.of_match = ds90ub92x_ids,
	.ofdata_to_platdata = ds90ub92x_ofdata_to_platdata,
	/* .platdata_auto_alloc_size = sizeof(struct ds90ub92x_platdata), */
	.bind = ds90ub92x_bind,
	.probe = ds90ub92x_probe,
	.ops	= &ds90ub92x_i2c_ops,
};

