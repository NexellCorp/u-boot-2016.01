/*
 * (C) Copyright 2018 Nexell
 *
 * Hyejung, Kwon <cjscld15@nexell.co.kr>
 *
 * * SPDX-License-Identifier:      GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <i2c.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/nx_gpio.h>
#include <nx_i2c_gpio.h>

DECLARE_GLOBAL_DATA_PTR;

enum {
	PIN_SDA = 0,
	PIN_SCL,
	PIN_MAX,
};

struct i2c_gpio_bus {
	int	udelay;
	struct gpio_desc gpios[PIN_MAX];
};

struct nx_i2c_gpio_desc {
	int num;
	int pad_func;
};

struct nx_i2c_gpio {
	int	addr;
	struct nx_i2c_gpio_desc gpios[PIN_MAX];
};

int nx_i2c_gpio_write(struct udevice *dev, unsigned int cmd,
		unsigned char *buffer, int len)
{
	struct nx_i2c_gpio *i2c = dev_get_priv(dev);
	struct udevice *bus = dev_get_parent(dev);
	struct dm_i2c_ops *ops = i2c_get_ops(bus);
	struct i2c_msg msg;
	uint8_t buf[2];
	int ret = 0;;

	msg.addr = i2c->addr;
	msg.buf = buf;
	msg.len = 2;
	msg.flags = DM_I2C_CHIP_WR_ADDRESS;

	buf[0] = cmd;
	buf[1] = buffer[0];

	ret = ops->xfer(bus, &msg, 1);
	return ret;
}

struct udevice* nx_i2c_gpio_init(void)
{
	struct udevice *dev = NULL;
	int ret = 0;

	ret = uclass_get_device(UCLASS_NX_I2C_GPIO, 0, &dev);
	if (ret) {
		printf("[%s] failed to get i2c device\n", __func__);
		return dev;
	}
	return dev;
}

static int nx_i2c_gpio_probe(struct udevice *dev)
{
	struct udevice *parent = NULL;
	struct nx_i2c_gpio *i2c = dev_get_priv(dev);
	struct dm_i2c_ops *ops = NULL;
	int ret = 0, type = 0, num = 0, pad = 0;

	type = i2c->gpios[PIN_SDA].num / nx_gpio_max_bit;
	num = i2c->gpios[PIN_SDA].num % nx_gpio_max_bit;
	pad = i2c->gpios[PIN_SDA].pad_func;
	nx_gpio_set_pad_function(type, num, pad);

	type = i2c->gpios[PIN_SCL].num / nx_gpio_max_bit;
	num = i2c->gpios[PIN_SCL].num % nx_gpio_max_bit;
	pad = i2c->gpios[PIN_SCL].pad_func;
	nx_gpio_set_pad_function(type, num, pad);

	parent = dev_get_parent(dev);
	ops = i2c_get_ops(parent);
	if (!ops) {
		printf("[%s] failed to get ops\n", __func__);
		return -ENODEV;
	}
	ret = ops->probe_chip(parent, i2c->addr, 0);
	if (ret) {
		debug("[%s] failed to probe chip\n", __func__);
		dev = NULL;
	}
	return ret;
}

static int nx_i2c_gpio_ofdata_to_platdata(struct udevice *dev)
{
	struct udevice *bus = dev_get_parent(dev);
	struct i2c_gpio_bus *p_i2c = dev_get_priv(bus);
	struct nx_i2c_gpio *i2c = dev_get_priv(dev);
	const void *blob = gd->fdt_blob;
	int ret = 0;

	i2c->gpios[PIN_SDA].num = gpio_get_number(&p_i2c->gpios[PIN_SDA]);
	i2c->gpios[PIN_SCL].num = gpio_get_number(&p_i2c->gpios[PIN_SCL]);

	if (dev->of_offset < 0) {
		printf("[%s] of_offset is invalid\n", __func__);
		return -EINVAL;
	}
	if ((i2c->gpios[PIN_SDA].num < 0) ||
			(i2c->gpios[PIN_SCL].num < 0)) {
		debug("[%s] failed to get gpios information\n", __func__);
		return -EINVAL;
	}
	i2c->addr = fdtdec_get_int(blob, dev->of_offset, "reg", 5);
	i2c->gpios[PIN_SDA].pad_func =
		fdtdec_get_int(blob, dev->of_offset, "sda_pad", 5);
	i2c->gpios[PIN_SCL].pad_func =
		fdtdec_get_int(blob, dev->of_offset, "scl_pad", 5);
	debug("[ADDR:%x], [SDA:%d-%d] [SCL:%d-%d]\n",
			i2c->addr,
			i2c->gpios[PIN_SDA].num,
			i2c->gpios[PIN_SDA].pad_func,
			i2c->gpios[PIN_SDA].num,
			i2c->gpios[PIN_SDA].pad_func);
	return ret;
}


static const struct udevice_id nx_i2c_gpio_ids[] = {
	{ .compatible = "nexell,nx_i2c_gpio" },
	{}
};

U_BOOT_DRIVER(nx_i2c_gpio) = {
	.name = "nx_i2c_gpio",
	.id = UCLASS_NX_I2C_GPIO,
	.of_match = nx_i2c_gpio_ids,
	.probe = nx_i2c_gpio_probe,
	.ofdata_to_platdata = nx_i2c_gpio_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct nx_i2c_gpio),
};

UCLASS_DRIVER(nx_i2c_gpio) = {
	.id = UCLASS_NX_I2C_GPIO,
	.name = "nx_i2c_gpio",
};

