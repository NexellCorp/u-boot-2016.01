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
#include <ub927928.h>

/* #define msleep(a) udelay(a * 1000) */

#define i2c_send_slave_addr 0x0c /* 927 */
#define i2c_rev_slave_addr 0x2c /* 928 */

struct ub927928_platdata {
	struct gpio_desc gpio_en;
};

static int ub9xx_i2c_read(struct udevice *dev,
	unsigned char slave_addr, unsigned int cmd,
	unsigned char *buf, int len)
{
	struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);

	chip->chip_addr = slave_addr;

	return dm_i2c_read(dev, cmd, buf, len);
}

static int ub9xx_i2c_write(struct udevice *dev,
	unsigned char slave_addr, unsigned int cmd,
	unsigned char *buf, int len)
{
	struct dm_i2c_chip *chip = dev_get_parent_platdata(dev);

	chip->chip_addr = slave_addr;
	chip->flags = DM_I2C_CHIP_WR_ADDRESS;

	return dm_i2c_write(dev, cmd, buf, len);
}

int ub9xx_set_mode(struct udevice *dev , int mode)
{
	return 0;
}

int ub9xx_init(struct udevice *dev)
{
	int i;
	int ret;
	unsigned char data[1] = {0};

	u16 reg[] = {0x17, 0x0D, 0x0E, 0x0F, 0x18, 0x19, 0x11, 0x10};
	u16 val[] = {0x9e, 0x03, 0x53, 0x03, 0x21, 0x25, 0x03, 0x03};

	u16 reg1[] = {0x1D, 0x1E, 0x1F, 0x26, 0x27/*add*/, 0x20, 0x21};
	u16 val1[] = {0x05, 0x35, 0x05, 0x21, 0x25/*add*/, 0x90, 0x90};

	/*
	unsigned char temp = 0x3;

	ret = ub9xx_i2c_write(dev, i2c_send_slave_addr, 0x01, &temp, 1);
	printf("ti927_reset: ret: %d\n", ret);

	ret = ub9xx_i2c_write(dev, i2c_rev_slave_addr, 0x01, &temp, 1);
	printf("ti928_reset: ret: %d\n", ret);

	mdelay(100);
	*/

	for (i = 0; i < 3; i++) {
		ret = ub9xx_i2c_read(dev, i2c_rev_slave_addr, 0x1c, data, 1);
#ifndef QUICKBOOT
		printf("ti927_Init: read reg:0x1c: 0x%x\n", data[0]);
#endif

		if (data[0] == 0x03) {
			printf("927,928 already lock\n");
			break;
		} else if (data[0] == -1) {
			printf("read 928 error\n");
			if (i == 9)
				return -1;
			break;
		}

		mdelay(1);
	}

	ret = ub9xx_i2c_read(dev, i2c_rev_slave_addr, 0x21, data, 1);
#ifndef QUICKBOOT
	printf("ti927_Init: read reg:0x21: 0x%x\n", data[0]);
#endif

	if ((data[0] & 0xf0) == 0x90) {
		printf("927,928 already init\n");
		return;
	}

	/* ub9xx_set_mode(dev , E_E522XX_PORT_CONF_CDP); */

	/* 927 write */
	for (i = 0; i < ARRAY_SIZE(reg); i++) {
		ret = ub9xx_i2c_write(dev, i2c_send_slave_addr,
				reg[i], &val[i], 1);
		mdelay(1);
	}

	/* 928 write */
	for (i = 0; i < ARRAY_SIZE(reg1); i++)
		ret = ub9xx_i2c_write(dev, i2c_rev_slave_addr,
				reg1[i], &val1[i], 1);

#ifndef QUICKBOOT
	printf("ub927928_init-----ret[%d]\n" , ret);
#endif

	return 0;
}
static int ub927928_probe(struct udevice *dev)
{
#ifndef QUICKBOOT
	printf("ub927928_probe\n");
#endif

	return 0;
}
static int ub927928_ofdata_to_platdata(struct udevice *dev)
{
	/*
	struct ub927928_platdata *pdata = dev->platdata;
	int ret;

	ret = gpio_request_by_name(dev, "gpios-en",
		0, &(pdata->gpio_en), GPIOD_IS_OUT);
	if (ret) {
		debug("%s: Could not decode sleep-gpios (%d)\n", __func__, ret);
		return ret;
	}

	dm_gpio_set_value(&pdata->gpio_en , 1);
	printf("ub927928_ofdata_to_platdata[%d]\n", pdata->gpio_en.offset);
	*/

	return 0;
}

static int ub927928_bind(struct udevice *dev)
{
	/*
	struct gpio_desc power_en;
	gpio_request_by_name(dev, "gpios-en", 0, &power_en,GPIOD_IS_OUT);
	dm_gpio_set_value(&power_en , 0);
	dm_i2c_read(dev,0x01, &value, 1);
	*/

#ifndef QUICKBOOT
	printf("ub927928_bind\n");
#endif

	return 0;
}

static const struct udevice_id ub927928_ids[] = {
	{ .compatible = "mediatek,ub9xx" },
	{}
};

/*
struct ub927928_ops {
	int (*init)(struct udevice *dev);
	int (*set_mode)(struct udevice *dev, int mode);
};
int ub927928_init(struct udevice *dev);
int ub927928_set_mode(struct udevice *dev, int mode);
*/

static struct ub927928_ops ub9xx_ops = {
	.init = ub9xx_init,
	.set_mode = ub9xx_set_mode,
};

U_BOOT_DRIVER(artik_ub9xx) = {
	.name = "ub927928",
	.id = UCLASS_UB927928_ID,
	.of_match = ub927928_ids,
	.probe = ub927928_probe,
	.bind = ub927928_bind,
	.ofdata_to_platdata = ub927928_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct ub927928_platdata),
	.ops = &ub9xx_ops,
};

void ub927928_init_s(void)
{
	struct udevice *dev;
	int ret;
	const struct ub927928_ops *ops;

	ret = uclass_get_device(UCLASS_UB927928_ID, 0, &dev);
	if (ret)
		return;

	ops = device_get_ops(dev);
	if (ops->init)
		ret = ops->init(dev);
}

UCLASS_DRIVER(ub9xx) = {
	.id = UCLASS_UB927928_ID,
	.name = "ub9xx",
};

