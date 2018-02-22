/*
 * (C) Copyright 2017 ZhongHong
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
#include <nexell_i2c.h>

/* #define msleep(a) udelay(a * 1000) */

#define i2c_send_slave_addr 0x44 /* 927 */
#define i2c_rev_slave_addr 0x2c /* 928 */

struct nx_i2c_platdata {
	struct gpio_desc gpio_en;
};

static __attribute__((unused)) int nx_i2c_read(struct udevice *dev,
	unsigned char slave_addr, unsigned int cmd,
	unsigned char *buf, int len)
{
	struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);

	chip->chip_addr = slave_addr;

	return dm_i2c_read(dev, cmd, buf, len);
}

static int nx_i2c_write(struct udevice *dev,
	unsigned char slave_addr, unsigned int cmd,
	unsigned char *buf, int len)
{
	struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);

	chip->chip_addr = slave_addr;
	chip->flags = DM_I2C_CHIP_WR_ADDRESS;

	return dm_i2c_write(dev, cmd, buf, len);
}

int nx_i2c_set_mode(struct udevice *dev , int mode)
{
	return 0;
}

int nx_i2c_init(struct udevice *dev)
{
	return 0;
}

static int nx_i2c_probe(struct udevice *dev)
{
	return 0;
}

static int nx_i2c_ofdata_to_platdata(struct udevice *dev)
{
	return 0;
}

static int nx_i2c_bind(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id nx_i2c_ids[] = {
	{ .compatible = "nexell,nx_i2c" },
	{}
};

static struct nx_i2c_ops i2c_ops = {
	.init = nx_i2c_init,
	.set_mode = nx_i2c_set_mode,
};

U_BOOT_DRIVER(nx_i2c) = {
	.name = "nx_i2c",
	.id = UCLASS_NX_I2C_ID,
	.of_match = nx_i2c_ids,
	.probe = nx_i2c_probe,
	.bind = nx_i2c_bind,
	.ofdata_to_platdata = nx_i2c_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct nx_i2c_platdata),
	.ops = &i2c_ops,
};

void nx_i2c_init_s(u8 *reg, u8 *val)
{
	struct udevice *dev;
	int ret, i;

	ret = uclass_get_device(UCLASS_NX_I2C_ID, 0, &dev);
	if (ret)
		return;
	for (i = 0; i < ARRAY_SIZE(reg); i++) {
		ret = nx_i2c_write(dev, i2c_send_slave_addr,
				reg[i], &val[i], 1);
	}
}

UCLASS_DRIVER(nx_i2c) = {
	.id = UCLASS_NX_I2C_ID,
	.name = "nx_i2c",
};

