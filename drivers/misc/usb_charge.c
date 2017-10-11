#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <i2c.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <usb_charge.h>

enum e522xx_state {
	E_E522XX_STATE_SLEEP = 0,
	E_E522XX_STATE_STANDBY,
	E_E522XX_STATE_RUN,
	E_E522XX_STATE_ERROR
};

enum e522xx_port_conf {
	E_E522XX_PORT_CONF_SDP = 0,
	E_E522XX_PORT_CONF_CDP,
	E_E522XX_PORT_CONF_DCP,
	E_E522XX_PORT_CONF_CCP_APPLE,
	E_E522XX_PORT_CONF_CCP,
	E_E522XX_PORT_CONF_AUTO
};

struct usb_charge_platdata {
	struct gpio_desc gpio_en;
};

static void e522xx_en_pin_ctrl(struct udevice *dev, int state)
{
	struct usb_charge_platdata *pdata = dev->platdata;

	if (state == 0)
		dm_gpio_set_value(&pdata->gpio_en, 0);
	else
		dm_gpio_set_value(&pdata->gpio_en, 1);
}

static void e522xx_state_ctrl(struct udevice *dev , int state)
{
	switch (state) {
	case E_E522XX_STATE_SLEEP:
		e522xx_en_pin_ctrl(dev, 0);
		break;

	case E_E522XX_STATE_STANDBY:
		/*
		e522xx_en_pin_ctrl(dev, 0);
		mdelay(100);
		*/
		e522xx_en_pin_ctrl(dev, 1);
		udelay(500);
		e522xx_en_pin_ctrl(dev, 0);
		break;

	case E_E522XX_STATE_RUN:
		e522xx_en_pin_ctrl(dev, 1);
		break;

	case E_E522XX_STATE_ERROR:
		break;

	default:
		break;
	}
}

static int e522xx_i2c_read(struct udevice *dev,
	unsigned int cmd, unsigned char *buf, int len)
{
	return dm_i2c_read(dev, cmd, buf, len);
}

static int e522xx_i2c_write(struct udevice *dev,
	unsigned int cmd, unsigned char *buf, int len)
{
	return dm_i2c_write(dev, cmd, buf, len);
}

int e522xx_set_mode(struct udevice *dev , int mode)
{
	int ret = 0;
	unsigned char buf[2] = {0};
	unsigned int cmd;

	if (mode > E_E522XX_PORT_CONF_CCP || mode < 0)
		return -1;

	printf("e522xx_set_mode [%d]\n", mode);
	e522xx_state_ctrl(dev, E_E522XX_STATE_STANDBY);

	/* status */
	cmd = 0x10;
	ret = e522xx_i2c_read(dev, cmd, buf, 1);
	printf("read status reg 0x%x value=0x%x\n", cmd, buf[0]);

	/* SMPS */
	cmd = 0x1A;
	/* (>4.0A) 0x8f--nrp off   0xff--nrp 400  bit13=1(5.25v) */
	buf[0] = 0x01;
	buf[1] = 0x8f;
	ret = e522xx_i2c_write(dev, cmd, buf, 2);
	if (ret)
		printf("usb_i2c_Write fail1==%d\n", ret);

	switch (mode) {
	case E_E522XX_PORT_CONF_SDP:
		/* BC1.2 */
		cmd = 0x01;
		/* 0x04(SDP) 0x0c(DCP-BC1.2)  0X10(CCP FOR APPLE) 0x08(CDP) */
		buf[0] = 0x04;
		ret = e522xx_i2c_write(dev, cmd, buf, 1);
		if (ret)
			printf("usb_i2c_Write fail2==%d\n", ret);
		break;

	case E_E522XX_PORT_CONF_CDP:
		/* BC1.2 */
		cmd = 0x01;
		/* 0x0c(DCP-BC1.2)  0X10(CCP FOR APPLE) 0x08(CDP) */
		buf[0] = 0x08;
		ret = e522xx_i2c_write(dev, cmd, buf, 1);
		if (ret)
			printf("usb_i2c_Write fail2==%d\n", ret);
		break;

	case E_E522XX_PORT_CONF_DCP:
		/* BC1.2 */
		cmd = 0x01;
		/* 0x0c(DCP-BC1.2)  0X10(CCP FOR APPLE) 0x08(CDP)  0x14(CCP) */
		buf[0] = 0x0c;
		printf("DCP-BC1.2\n");
		ret = e522xx_i2c_write(dev, cmd, buf, 1);
		if (ret)
			printf("usb_i2c_Write fail2==%d\n", ret);
		break;

	case E_E522XX_PORT_CONF_CCP_APPLE:
		/* BC1.2 */
		cmd = 0x01;
		/* 0x0c(DCP-BC1.2)  0X10(CCP FOR APPLE) 0x08(CDP)  0x14(CCP) */
		buf[0] = 0x10;
		printf("CCP APPLE\n");
		ret = e522xx_i2c_write(dev, cmd, buf, 1);
		if (ret)
			printf("usb_i2c_Write fail2==%d\n", ret);
		break;

	case E_E522XX_PORT_CONF_CCP:
		/* BC1.2 */
		cmd = 0x01;
		/* 0x0c(DCP-BC1.2)  0X10(CCP FOR APPLE) 0x08(CDP)  0x14(CCP) */
		buf[0] = 0x14;
		printf("CCP\n");
		ret = e522xx_i2c_write(dev, cmd, buf, 1);
		if (ret)
			printf("usb_i2c_Write fail2==%d\n", ret);
		break;

	case E_E522XX_PORT_CONF_AUTO:
		/* BC1.2 */
		cmd = 0x01;
		/* 0x0c(DCP-BC1.2)  0X10(CCP FOR APPLE) 0x08(CDP)  0x14(CCP) */
		buf[0] = 0x1c;
		printf("AUTO\n");
		ret = e522xx_i2c_write(dev, cmd, buf, 1);
		if (ret)
			printf("usb_i2c_Write fail2==%d\n", ret);
		break;

	default:
		break;
	}

	e522xx_state_ctrl(dev, E_E522XX_STATE_RUN);
	if (ret)
		printf("e522xx_set_mode [%d] fail\n", mode);
	else
		printf("e522xx_set_mode [%d] success\n", mode);

	return ret;
}

int e522_init(struct udevice *dev)
{
	/* uint16_t value; */

	e522xx_set_mode(dev , E_E522XX_PORT_CONF_CDP);
	/* dm_i2c_read(dev,0x84, &value, 1); */
	printf("usb_charge_init\n");

	return 0;
}

int e522_set_mode(struct udevice *dev, int mode)
{
	/* printf("usb_charge_set_mode\n"); */

	return 0;
}

static int usb_charge_probe(struct udevice *dev)
{
	/* printf("usb_charge_probe\n"); */

	return 0;
}

static int usb_charge_ofdata_to_platdata(struct udevice *dev)
{
	struct usb_charge_platdata *pdata = dev->platdata;

	gpio_request_by_name(
		dev, "gpios-en", 0, &(pdata->gpio_en), GPIOD_IS_OUT);
	dm_gpio_set_value(&pdata->gpio_en , 0);

	return 0;
}

static int usb_charge_bind(struct udevice *dev)
{
	/*
	struct gpio_desc power_en;
	uint8_t value = 0 ;
	gpio_request_by_name(dev, "gpios-en", 0, &power_en,GPIOD_IS_OUT);
	dm_gpio_set_value(&power_en , 0);
	dm_i2c_read(dev,0x01, &value, 1);
	printf("usb_charge_bind\n");
	*/

	return 0;
}

static const struct udevice_id usb_charge_ids[] = {
	{ .compatible = "mediatek,e522xx" },
	{}
};

/*
struct usb_charge_ops {
	int (*init)(struct udevice *dev);
	int (*set_mode)(struct udevice *dev, int mode);
};
int usb_charge_init(struct udevice *dev);
int usb_charge_set_mode(struct udevice *dev, int mode);
*/

static struct usb_charge_ops e522_ops = {
	.init = e522_init,
	.set_mode = e522_set_mode,
};

U_BOOT_DRIVER(usb_charge) = {
	.name = "usb_charge",
	.id = UCLASS_USBCHARGE_ID,
	.of_match = usb_charge_ids,
	.probe = usb_charge_probe,
	.bind = usb_charge_bind,
	.ofdata_to_platdata = usb_charge_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct usb_charge_platdata),
	.ops = &e522_ops,
};

void usb_charge_init_s(void)
{
	struct udevice *dev;
	int ret;
	const struct usb_charge_ops *ops;

	ret = uclass_get_device(UCLASS_USBCHARGE_ID, 0, &dev);
	if (ret)
		return;

	ops = device_get_ops(dev);
	if (ops->init)
		ret = ops->init(dev);
}

UCLASS_DRIVER(usb_charge_id) = {
	.id = UCLASS_USBCHARGE_ID,
	.name = "usb_chargeid",
};

