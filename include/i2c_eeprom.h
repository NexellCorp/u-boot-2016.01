/*
 * Copyright (c) 2014 Google, Inc
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __I2C_EEPROM
#define __I2C_EEPROM

struct i2c_eeprom_ops {
	int (*read)(struct udevice *dev, int offset, uint8_t *buf, int size);
	int (*write)(struct udevice *dev, int offset, const uint8_t *buf,
		     int size);
};

struct i2c_eeprom {
	int addr;
	int olen;
	unsigned long pagesize;
	unsigned pagewidth;
};

struct dm_eeprom_uclass_platdata {
	const char *name;
};

int dm_eeprom_read(struct udevice *dev, int offset, uint8_t *buf, int size);
int dm_eeprom_write(struct udevice *dev, int offset,
		const uint8_t *buf, int size);

#endif
