/*
 * nxe2000.c  --  PMIC driver for the NEXELL NXE2000
 *
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Jongshin, Park <pjsin865@nexell.co.kr>
 *
 * SPDX-License-Identifier:     GPL-2.0+

 */

#include <common.h>
#include <fdtdec.h>
#include <errno.h>
#include <dm.h>
#include <i2c.h>
#include <power/pmic.h>
#include <power/regulator.h>
#include <power/nxe2000.h>

DECLARE_GLOBAL_DATA_PTR;

static const struct pmic_child_info pmic_children_info[] = {
	{ .prefix = "LDO", .driver = NXE2000_LDO_DRIVER },
	{ .prefix = "BUCK", .driver = NXE2000_BUCK_DRIVER },
	{ },
};

static int nxe2000_reg_count(struct udevice *dev)
{
	return NXE2000_NUM_OF_REGS;
}

static int nxe2000_write(struct udevice *dev, uint reg, const uint8_t *buff,
			  int len)
{
	if (dm_i2c_write(dev, reg, buff, len)) {
		error("write error to device: %p register: %#x!", dev, reg);
		return -EIO;
	}

	return 0;
}

static int nxe2000_read(struct udevice *dev, uint reg, uint8_t *buff, int len)
{
	if (dm_i2c_read(dev, reg, buff, len)) {
		error("read error from device: %p register: %#x!", dev, reg);
		return -EIO;
	}

	return 0;
}

static int nxe2000_bind(struct udevice *dev)
{
	int regulators_node;
	const void *blob = gd->fdt_blob;
	int children;

	regulators_node = fdt_subnode_offset(blob, dev->of_offset,
					     "voltage-regulators");
	if (regulators_node <= 0) {
		debug("%s: %s regulators subnode not found!", __func__,
							     dev->name);
		return -ENXIO;
	}

	debug("%s: '%s' - found regulators subnode\n", __func__, dev->name);

	children = pmic_bind_children(dev, regulators_node, pmic_children_info);
	if (!children)
		debug("%s: %s - no child found\n", __func__, dev->name);

	/* Always return success for this device */
	return 0;
}

static struct dm_pmic_ops nxe2000_ops = {
	.reg_count = nxe2000_reg_count,
	.read = nxe2000_read,
	.write = nxe2000_write,
};

static const struct udevice_id nxe2000_ids[] = {
	{ .compatible = "nexell,nxe2000" },
	{ }
};

U_BOOT_DRIVER(pmic_nxe2000) = {
	.name = "nxe2000_pmic",
	.id = UCLASS_PMIC,
	.of_match = nxe2000_ids,
	.bind = nxe2000_bind,
	.ops = &nxe2000_ops,
};
