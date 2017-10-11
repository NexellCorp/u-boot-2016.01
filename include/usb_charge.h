#ifndef __USB_CHARGE_H
#define __USB_CHARGE_H

struct usb_charge_ops {
	int (*init)(struct udevice *dev);
	int (*set_mode)(struct udevice *dev, int mode);
};

int usb_charge_init(struct udevice *dev);
int usb_charge_set_mode(struct udevice *dev, int mode);
void usb_charge_init_s(void);
#endif
