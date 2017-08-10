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
#ifdef CONFIG_REVISION_TAG
#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static const struct pmic_child_info pmic_reg_info[] = {
	{ .prefix = "BUCK", .driver = NXE2000_BUCK_DRIVER },
	{ .prefix = "LDO", .driver = NXE2000_LDO_DRIVER },
	{ },
};

static const struct pmic_child_info pmic_chg_info[] = {
	{ .prefix = "CHG", .driver = NXE2000_CHG_DRIVER },
	{ },
};

#ifdef CONFIG_REVISION_TAG
static void gpio_init(void)
{
	nx_gpio_set_pad_function(4, 4, 0);
	nx_gpio_set_pad_function(4, 5, 0);
	nx_gpio_set_pad_function(4, 6, 0);
	nx_gpio_set_output_value(4, 4, 0);
	nx_gpio_set_output_value(4, 5, 0);
	nx_gpio_set_output_value(4, 6, 0);
}

static u32 hw_revision(void)
{
	u32 val = 0;

	val |= nx_gpio_get_input_value(4, 6);
	val <<= 1;

	val |= nx_gpio_get_input_value(4, 5);
	val <<= 1;

	val |= nx_gpio_get_input_value(4, 4);

	return val;
}
#endif

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

#if defined(CONFIG_PMIC_REG_DUMP)
static int nxe2000_reg_dump(struct udevice *dev, const char *title)
{
	int i;
	int ret = 0;
	uint8_t value = 0;

	printf("############################################################\n");
	printf("##\e[31m %s \e[0m\n", title);
	printf("##       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F\n");
	for (i = 0; i <= NXE2000_NUM_OF_REGS; i++) {
		if (i%16 == 0)
			printf("##  %02X:", i);
		if (i%4 == 0)
			printf(" ");
		ret = dm_i2c_read(dev, i, &value, 1);
		if (!ret)
			printf("%02x ", value);
		else
			printf("\e[31mxx\e[0m ");
		if ((i+1)%16 == 0)
			printf("\n");
	}
	printf("############################################################\n");

	return 0;
}
#endif

static int nxe2000_param_setup(struct udevice *dev, uint8_t *cache)
{
	struct dm_nxe2000_platdata *pdata = dev->platdata;
	uint8_t value = 0x0;

	/* for Key error. */
	dm_i2c_write(dev, NXE2000_REG_ADCCNT3, &value, 1);

	/* Set GPIO4 Condition : High(Hi-Z)*/
	dm_i2c_read(dev, NXE2000_REG_IOOUT, &value, 1);
	value |= 0x10;
	dm_i2c_write(dev, NXE2000_REG_IOOUT, &value, 1);

#if defined(CONFIG_SW_UBC_DETECT)
	/* Set GPIO4 Direction : input*/
	dm_i2c_read(dev, NXE2000_REG_IOSEL, &value, 1);
	value &= ~0x10;
	dm_i2c_write(dev, NXE2000_REG_IOSEL, &value, 1);
#else
	/* Set GPIO4 Direction : output*/
	dm_i2c_read(dev, NXE2000_REG_IOSEL, &value, 1);
	value |= 0x10;
	dm_i2c_write(dev, NXE2000_REG_IOSEL, &value, 1);
#endif

	dm_i2c_read(dev, NXE2000_REG_PWRONTIMSET
		, &cache[NXE2000_REG_PWRONTIMSET], 1);
	cache[NXE2000_REG_PWRONTIMSET] &=
		~(0x7 << NXE2000_POS_PWRONTIMSET_OFF_PRESS_PWRON);
	cache[NXE2000_REG_PWRONTIMSET] |=
		(pdata->off_press_time
			<< NXE2000_POS_PWRONTIMSET_OFF_PRESS_PWRON);

	cache[NXE2000_REG_CHGCTL1] =
		((0 << NXE2000_POS_CHGCTL1_CHGP)
		| (0 << NXE2000_POS_CHGCTL1_CHGCMP_DIS)
		| (1 << NXE2000_POS_CHGCTL1_NOBATOVLIM)
		| (0 << NXE2000_POS_CHGCTL1_OTG_BOOST_EN)
		| (0 << NXE2000_POS_CHGCTL1_SUSPEND)
		| (0 << NXE2000_POS_CHGCTL1_JEITAEN)
		| (0 << NXE2000_POS_CHGCTL1_VUSBCHGEN)
		| (0 << NXE2000_POS_CHGCTL1_VADPCHGEN));

	cache[NXE2000_REG_CHGCTL2] =
	((pdata->chg_usb_vcontmask << NXE2000_POS_CHGCTL2_USB_VCONTMASK)
	| (pdata->chg_adp_vcontmask << NXE2000_POS_CHGCTL2_ADP_VCONTMASK)
	| (pdata->chg_vbus_buck_ths << NXE2000_POS_CHGCTL2_VUSBBUCK_VTH)
	| (pdata->chg_vadp_buck_ths << NXE2000_POS_CHGCTL2_VADPBUCK_VTH));

	cache[NXE2000_REG_VSYSSET] =
		((pdata->vsys_vol << NXE2000_POS_VSYSSET_VSYSSET)
		| (pdata->vsys_over_vol << NXE2000_POS_VSYSSET_VSYSOVSET));

	cache[NXE2000_REG_DIESET] =
		((pdata->die_return_temp << NXE2000_POS_DIESET_DIERTNTEMP)
		| (pdata->die_error_temp << NXE2000_POS_DIESET_DIEERRTEMP)
		| (pdata->die_shutdown_temp << NXE2000_POS_DIESET_DIESHUTTEMP));

	cache[NXE2000_REG_REPCNT] =
		((pdata->repcnt_off_reseto << NXE2000_POS_REPCNT_OFF_RESETO)
		| (pdata->repcnt_repwrtim << NXE2000_POS_REPCNT_REPWRTIM)
		| (pdata->repcnt_repwron << NXE2000_POS_REPCNT_REPWRON));

	cache[NXE2000_REG_WATCHDOG] =
		((pdata->wdog_slpen << NXE2000_POS_WDOG_SLPEN)
		| (pdata->wdog_en << NXE2000_POS_WDOG_EN)
		| (pdata->wdog_tim << NXE2000_POS_WDOG_TIM));

	cache[NXE2000_REG_PWRIREN] =
		((pdata->pwrien_wdog << NXE2000_POS_PWRIREN_WDOG)
		| (pdata->pwrien_noe_off << NXE2000_POS_PWRIREN_NOE_OFF)
		| (pdata->pwrien_pwron_off << NXE2000_POS_PWRIREN_PWRON_OFF)
		| (pdata->pwrien_preot << NXE2000_POS_PWRIREN_PREOT)
		| (pdata->pwrien_prvindt << NXE2000_POS_PWRIREN_PRVINDT)
		| (pdata->pwrien_extin << NXE2000_POS_PWRIREN_EXTIN)
		| (pdata->pwrien_pwron << NXE2000_POS_PWRIREN_PWRON));

	return 0;
}

static int nxe2000_device_setup(struct udevice *dev, uint8_t *cache)
{
	struct dm_nxe2000_platdata *pdata = dev->platdata;

	if (pdata->off_press_time != -ENODATA)
		dm_i2c_write(dev, NXE2000_REG_PWRONTIMSET
			, &cache[NXE2000_REG_PWRONTIMSET], 1);

	dm_i2c_write(dev, NXE2000_REG_CHGCTL1
		, &cache[NXE2000_REG_CHGCTL1], 1);

	if (
		(pdata->chg_usb_vcontmask != -ENODATA) &&
		(pdata->chg_adp_vcontmask != -ENODATA) &&
		(pdata->chg_vbus_buck_ths != -ENODATA) &&
		(pdata->chg_vadp_buck_ths != -ENODATA))
		dm_i2c_write(dev, NXE2000_REG_CHGCTL2
			, &cache[NXE2000_REG_CHGCTL2], 1);

	if (
		(pdata->vsys_vol != -ENODATA) &&
		(pdata->vsys_over_vol != -ENODATA))
		dm_i2c_write(dev, NXE2000_REG_VSYSSET
			, &cache[NXE2000_REG_VSYSSET], 1);

	if (
		(pdata->die_return_temp != -ENODATA) &&
		(pdata->die_error_temp != -ENODATA) &&
		(pdata->die_shutdown_temp != -ENODATA))
		dm_i2c_write(dev, NXE2000_REG_DIESET, &cache[NXE2000_REG_DIESET]
			, 1);

	if (
		(pdata->repcnt_off_reseto != -ENODATA) &&
		(pdata->repcnt_repwrtim != -ENODATA) &&
		(pdata->repcnt_repwron != -ENODATA))
		dm_i2c_write(dev, NXE2000_REG_REPCNT, &cache[NXE2000_REG_REPCNT]
			, 1);

	if (
		(pdata->wdog_slpen != -ENODATA) &&
		(pdata->wdog_en != -ENODATA) &&
		(pdata->wdog_tim != -ENODATA))
		dm_i2c_write(dev, NXE2000_REG_WATCHDOG
			, &cache[NXE2000_REG_WATCHDOG], 1);

	if (
		(pdata->pwrien_wdog != -ENODATA) &&
		(pdata->pwrien_noe_off != -ENODATA) &&
		(pdata->pwrien_pwron_off != -ENODATA) &&
		(pdata->pwrien_preot != -ENODATA) &&
		(pdata->pwrien_prvindt != -ENODATA) &&
		(pdata->pwrien_extin != -ENODATA) &&
		(pdata->pwrien_pwron != -ENODATA))
		dm_i2c_write(dev, NXE2000_REG_PWRIREN
			, &cache[NXE2000_REG_PWRIREN], 1);

	return 0;
}

static int nxe2000_probe(struct udevice *dev)
{
	uint8_t value = 0x0;
	uint8_t cache[256] = {0x0,};

	dm_i2c_write(dev, NXE2000_REG_BANKSEL, &value, 1);

#if defined(CONFIG_PMIC_REG_DUMP)
	nxe2000_reg_dump(dev, "PMIC Register Dump");
#endif

	nxe2000_param_setup(dev, cache);
	nxe2000_device_setup(dev, cache);

#if defined(CONFIG_PMIC_REG_DUMP)
	nxe2000_reg_dump(dev, "PMIC Setup Register Dump");
#endif
	return 0;
}

static int nxe2000_ofdata_to_platdata(struct udevice *dev)
{
	struct dm_nxe2000_platdata *pdata = dev->platdata;
	const void *blob = gd->fdt_blob;
	int nxe2000_node;

	nxe2000_node = fdt_subnode_offset(blob,
		fdt_path_offset(blob, "/"),	"init-nxe2000");

	pdata->off_press_time = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,off_press_time", -ENODATA);

	pdata->chg_usb_vcontmask = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,chg_usb_vcontmask", -ENODATA);
	pdata->chg_adp_vcontmask = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,chg_adp_vcontmask", -ENODATA);
	pdata->chg_vbus_buck_ths = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,chg_vbus_buck_ths", -ENODATA);
	pdata->chg_vadp_buck_ths = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,chg_vadp_buck_ths", -ENODATA);

	pdata->vsys_vol = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,vsys_vol", -ENODATA);
	pdata->vsys_over_vol = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,vsys_over_vol", -ENODATA);

	pdata->die_return_temp = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,die_return_temp", -ENODATA);
	pdata->die_error_temp = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,die_error_temp", -ENODATA);
	pdata->die_shutdown_temp = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,die_shutdown_temp", -ENODATA);

	pdata->repcnt_off_reseto = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,repcnt_off_reseto", -ENODATA);
	pdata->repcnt_repwrtim = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,repcnt_repwrtim", -ENODATA);
	pdata->repcnt_repwron = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,repcnt_repwron", -ENODATA);

	pdata->wdog_slpen = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,wdog_slpen", -ENODATA);
	pdata->wdog_tim = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,wdog_tim", -ENODATA);

	pdata->pwrien_wdog = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,pwrien_wdog", -ENODATA);
	pdata->pwrien_noe_off = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,pwrien_noe_off", -ENODATA);
	pdata->pwrien_pwron_off = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,pwrien_pwron_off", -ENODATA);
	pdata->pwrien_preot = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,pwrien_preot", -ENODATA);
	pdata->pwrien_prvindt = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,pwrien_prvindt", -ENODATA);
	pdata->pwrien_extin = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,pwrien_extin", -ENODATA);
	pdata->pwrien_pwron = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,pwrien_pwron", -ENODATA);

	pdata->adc_ain0 = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,adc_ain0", -ENODATA);
	pdata->adc_ain1 = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,adc_ain1", -ENODATA);
	pdata->adc_vthm = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,adc_vthm", -ENODATA);
	pdata->adc_vsys = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,adc_vsys", -ENODATA);
	pdata->adc_vusb = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,adc_vusb", -ENODATA);
	pdata->adc_vadp = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,adc_vadp", -ENODATA);
	pdata->adc_vbat = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,adc_vbat", -ENODATA);
	pdata->adc_ilim = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,adc_ilim", -ENODATA);

	pdata->adccnt3_adrq = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,adccnt3_adrq", -ENODATA);
	pdata->adccnt3_ave = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,adccnt3_ave", -ENODATA);
	pdata->adccnt3_adsel = fdtdec_get_int(blob, nxe2000_node,
		"nxe2000,adccnt3_adsel", -ENODATA);

	return 0;
}

static int nxe2000_bind(struct udevice *dev)
{
	int reg_node = 0;
	int chg_node = 0;
	const void *blob = gd->fdt_blob;
	int children;

#ifdef CONFIG_REVISION_TAG
	gpio_init();

	if ((hw_revision() < 3) && !strcmp(dev->name, "nxe2000_gpio@32"))
		return 0;

	if ((hw_revision() >= 3) && !strcmp(dev->name, "nxe2000@32"))
		return 0;
#endif
	debug("%s: dev->name:%s\n", __func__, dev->name);

	if (!strncmp(dev->name, "nxe2000", 7)) {
		reg_node = fdt_subnode_offset(blob, fdt_path_offset(blob, "/"),
				"voltage-regulators");
		chg_node = fdt_subnode_offset(blob, fdt_path_offset(blob, "/"),
				"init-charger");
	} else {
		reg_node = fdt_subnode_offset(blob, dev->of_offset,
				"voltage-regulators");
	}

	if (reg_node > 0) {
		debug("%s: found regulators subnode\n", __func__);
		children = pmic_bind_children(dev, reg_node, pmic_reg_info);
		if (!children)
			debug("%s: %s - no child found\n", __func__, dev->name);
	} else {
		debug("%s: regulators subnode not found!\n", __func__);
	}

	if (chg_node > 0) {
		debug("%s: found charger subnode\n", __func__);
		children = pmic_bind_children(dev, chg_node, pmic_chg_info);
		if (!children)
			debug("%s: %s - no child found\n", __func__, dev->name);
	} else {
		debug("%s: charger subnode not found!\n", __func__);
	}

	/* Always return success for this device */
	return 0;
}

static struct dm_pmic_ops nxe2000_ops = {
	.reg_count = nxe2000_reg_count,
	.read = nxe2000_read,
	.write = nxe2000_write,
};

static const struct udevice_id nxe2000_ids[] = {
	{ .compatible = "nexell,nxe1500", .data = (ulong)TYPE_NXE1500 },
	{ .compatible = "nexell,nxe2000", .data = (ulong)TYPE_NXE2000 },
	{ }
};

U_BOOT_DRIVER(pmic_nxe2000) = {
	.name = "nxe2000_pmic",
	.id = UCLASS_PMIC,
	.of_match = nxe2000_ids,
	.ofdata_to_platdata = nxe2000_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct dm_nxe2000_platdata),
	.bind = nxe2000_bind,
	.probe = nxe2000_probe,
	.ops = &nxe2000_ops,
};
