/*
 * Copyright (c) 2014 Google, Inc
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/err.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <i2c.h>
#include <i2c_eeprom.h>

DECLARE_GLOBAL_DATA_PTR;

static int i2c_eeprom_read(struct udevice *dev, int offset, uint8_t *buf,
			int size)
{
	struct i2c_eeprom *data = dev_get_priv(dev);
	struct udevice *chip;

	if (i2c_get_chip(dev->parent, data->addr, data->olen, &chip)) {
		printf("i2c_get_chip failed\n");
		return -1;
	}
	printf("data->olen = %x \n",data->olen);

	return dm_i2c_read(chip, offset, buf, size);
}

static int i2c_eeprom_write(struct udevice *dev, int offset,
			const uint8_t *buf, int size)
{
	struct i2c_eeprom *data = dev_get_priv(dev);
	struct udevice *chip;

	if (i2c_get_chip(dev->parent, data->addr, data->olen, &chip)) {
		printf("i2c_get_chip failed\n");
		return -1;
	}
	return dm_i2c_write(dev, offset, buf, size);
}

struct i2c_eeprom_ops i2c_eeprom_std_ops = {
	.read	= i2c_eeprom_read,
	.write	= i2c_eeprom_write,
};
static int i2c_eeprom_std_ofdata_to_platdata(struct udevice *dev)
{
	int addr, olen;
	struct i2c_eeprom *priv = dev_get_priv(dev);
	u64 data = dev_get_driver_data(dev);

	addr = fdtdec_get_int(gd->fdt_blob, dev->of_offset, "reg", 0);
	priv->addr = addr;

	olen = fdtdec_get_int(gd->fdt_blob, dev->of_offset,
			"u-boot,i2c-offset-len", 1);
	priv->olen = olen;

	/* 6 bit -> page size of up to 2^63 (should be sufficient) */
	priv->pagewidth = data & 0x3F;
	priv->pagesize = (1 << priv->pagewidth);

	return 0;
}

int i2c_eeprom_std_probe(struct udevice *dev)
{
	return 0;
}

static int i2c_eeprom_std_bind(struct udevice *dev)
{
	return 0;
}
static bool eeprom_name_is_unique(struct udevice *check_dev,
			const char *check_name)
{
	struct dm_eeprom_uclass_platdata *uc_pdata;
	struct udevice *dev;
	int check_len = strlen(check_name);
	int ret;
	int len;

	for (ret = uclass_find_first_device(UCLASS_I2C_EEPROM, &dev); dev;
		ret = uclass_find_next_device(&dev)) {
		if (ret || dev == check_dev)
			continue;

	uc_pdata = dev_get_uclass_platdata(dev);
	len = strlen(uc_pdata->name);

	if (len != check_len)
		continue;

	if (!strcmp(uc_pdata->name, check_name))
		return false;
	}

	return true;
}

int dm_eeprom_read(struct udevice *dev, int offset, uint8_t *buf, int size)
{
	const struct i2c_eeprom_ops *ops = dev_get_driver_ops(dev);

	if (!ops || !ops->read)
		return -ENOSYS;

	return ops->read(dev,offset,buf,size);
}

int dm_eeprom_write(struct udevice *dev, int offset,
		const uint8_t *buf, int size)
{
	const struct i2c_eeprom_ops *ops = dev_get_driver_ops(dev);

	if (!ops || !ops->write)
		return -ENOSYS;

	return ops->write(dev,offset,buf,size);

}

static int eeprom_post_bind(struct udevice *dev)
{
	struct dm_eeprom_uclass_platdata *uc_pdata;
	int offset = dev->of_offset;
	const void *blob = gd->fdt_blob;
	const char *property = "eeprom-name";

	uc_pdata = dev_get_uclass_platdata(dev);
	if (!uc_pdata)
		return -ENXIO;

	/* Charger mandatory constraint */
	uc_pdata->name = fdt_getprop(blob, offset, property, NULL);
	if (!uc_pdata->name) {
		debug("%s: dev: %s has no property 'eeprom-name'\n",
			__func__, dev->name);
		uc_pdata->name = fdt_get_name(blob, offset, NULL);
		if (!uc_pdata->name)
			return -EINVAL;
	}

	if (eeprom_name_is_unique(dev, uc_pdata->name))
		return 0;

	debug("\"%s\" of dev: \"%s\", has nonunique value: \"%s\"",
		property, dev->name, uc_pdata->name);

	return -EINVAL;
}

static int eeprom_pre_probe(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id i2c_eeprom_std_ids[] = {
	{ .compatible = "i2c-eeprom", .data = 0 },
	{ .compatible = "atmel,24c01a", .data = 3 },
	{ .compatible = "atmel,24c02", .data = 3 },
	{ .compatible = "atmel,24c04", .data = 4 },
	{ .compatible = "atmel,24c08a", .data = 4 },
	{ .compatible = "atmel,24c16a", .data = 4 },
	{ .compatible = "atmel,24c32", .data = 5 },
	{ .compatible = "atmel,24c64", .data = 5 },
	{ .compatible = "atmel,24c128", .data = 6 },
	{ .compatible = "atmel,24c256", .data = 6 },
	{ .compatible = "atmel,24c512", .data = 6 },
	{ .compatible = "fmd,24c256a", .data = 6 },
	{ }
};

U_BOOT_DRIVER(i2c_eeprom_std) = {
	.name			= "i2c_eeprom_std",
	.id			= UCLASS_I2C_EEPROM,
	.of_match		= i2c_eeprom_std_ids,
	.ops			= &i2c_eeprom_std_ops,
	.bind			= i2c_eeprom_std_bind,
	.probe			= i2c_eeprom_std_probe,
	.ofdata_to_platdata	= i2c_eeprom_std_ofdata_to_platdata,
	.priv_auto_alloc_size	= sizeof(struct i2c_eeprom),
};

UCLASS_DRIVER(i2c_eeprom) = {
	.id		= UCLASS_I2C_EEPROM,
	.name		= "i2c_eeprom",
	.post_bind  = eeprom_post_bind,
	.pre_probe  = eeprom_pre_probe,
	.per_device_platdata_auto_alloc_size =
		sizeof(struct dm_eeprom_uclass_platdata),
};
