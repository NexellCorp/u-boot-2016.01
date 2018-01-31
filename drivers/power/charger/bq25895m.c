/*
 * bq25895m.c  --  Charger driver for the NEXELL NXE2000
 *
 * Copyright (C) 2017  Nexell Co., Ltd.
 * Author: Jongshin, Park <allan.park@nexell.co.kr>
 *
 * SPDX-License-Identifier:     GPL-2.0+

 */

#include <common.h>
#include <fdtdec.h>
#include <errno.h>
#include <dm.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <power/pmic.h>
#include <power/charger.h>
#include <power/bq25895m.h>

DECLARE_GLOBAL_DATA_PTR;

static const struct udevice_id bq25895m_ids[] = {
    { .compatible = "ti,bq25895m" },
    {}
};

static int bq25895m_get_value_vbatt(struct udevice *dev)
{
	char data=0;
	dm_i2c_read(dev, 0xE, &data, 1);
	data &= ~(0x80);

	return 2304 + (data * 20);

}

static int bq25895m_get_charge_type(struct udevice *dev)
{
	char value=0;
	dm_i2c_read(dev, REG0B, &value, 1);

	return (int) value;
}


static const struct dm_charger_ops bq25895m_chg_ops = {
	.get_value_vbatt = bq25895m_get_value_vbatt,
	.get_charge_type = bq25895m_get_charge_type,
};

static int bq25895m_chg_bind(struct udevice *dev)
{
	return 0;
}
static int bq25895m_chg_probe(struct udevice *dev)
{
	char data=0, value=0;
	struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);
	struct dm_bq25895m_chg_platdata *pdata = dev->platdata;

	/*i2c md 6A 0 0x15 */
	/* Check ID */
	dm_i2c_read(dev, REG14, &value, 1);
	if( (value & 0x3) == 0x2)
		printf("FOUND BQ2489M Charger IC \n");
	else {
		printf("BQ2589m Not Found \n");
		return -EINVAL;
	}

	/* Temporary Set voltage*/
	if(pdata->chg_vol != 0) {
		value = (((pdata->chg_vol - 3840) >> 4));
		dm_i2c_read(dev, 0x6, &data, 1);
		data &= (0x3);
		data |= (value << 2);
		dm_i2c_write(dev,0x6, &data ,1);
	}


	return 0;
}

static int bq25895m_chg_ofdata_to_platdata(struct udevice *dev)
{
	struct dm_bq25895m_chg_platdata *pdata = dev->platdata;
	const void *blob = gd->fdt_blob;
	int offset = dev->of_offset;

	memset(pdata,0,sizeof(struct dm_bq25895m_chg_platdata));

	pdata->chg_vol = fdtdec_get_int(blob, offset,
			        "ti,charging-voltage", -ENODATA);
	gpio_request_by_name(
		dev, "stat-pin", 0, &(pdata->gpio_stat), GPIOD_IS_IN);

	if (!dm_gpio_is_valid(&(pdata->gpio_stat))) {
		printf("stat-gpio not valid \n");
		return;
	}

	gpio_request_by_name(
		dev, "irq-pin", 0, &(pdata->gpio_irq), GPIOD_IS_IN);

	if (!dm_gpio_is_valid(&(pdata->gpio_irq))) {
		printf("irq-gpio not valid \n");
		return;
	}

	return 0;
}

U_BOOT_DRIVER(bq25895m_charger) = {
	.name = BQ25895M_CHG_DRIVER,
	.id = UCLASS_CHARGER,
	.of_match = bq25895m_ids,
	.ops = &bq25895m_chg_ops,
	.bind = bq25895m_chg_bind,
	.probe = bq25895m_chg_probe,
	.ofdata_to_platdata = bq25895m_chg_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct dm_bq25895m_chg_platdata),
};
