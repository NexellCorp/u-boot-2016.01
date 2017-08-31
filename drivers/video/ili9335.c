/*
 * (C) 2017 Chanho Park <chanho61.park@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * Support ili9335 a-Si TFT LCD which is 240x320 resolution with SPI interface
 */

#include <common.h>
#include <dm.h>
#include <linux/err.h>
#include <video_fb.h>
#include <spi.h>
#include <lcd.h>
#include <asm/global_data.h>
#include <asm/arch/gpio.h>
#include <asm/gpio.h>

#include "videomodes.h"

DECLARE_GLOBAL_DATA_PTR;

#define MAX_SPI_BYTES 256	/* Maximum number of bytes we can handle */

vidinfo_t panel_info = {
	.vl_col = 320,
	.vl_row = 240,
	.vl_rot = 3,
	.vl_bpix = LCD_BPP,
};

struct spi_slave *slave;
uchar	txbuf[MAX_SPI_BYTES];
uchar	rxbuf[MAX_SPI_BYTES];

#define START_BYTE	0x70
#define ROTATE		270
#define WIDTH		240
#define HEIGHT		320
#define BGR		1
#define BPP		16
#define GAMMA_NUM	2
#define GAMMA_LEN	10
#define VMEM_SIZE	(WIDTH * HEIGHT * BPP / 8)
#define TXBUF_LEN	(VMEM_SIZE + 2)
#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))

unsigned long gamma_curves[GAMMA_NUM * GAMMA_LEN] = {
	7, 7, 6, 0, 0, 0, 5, 5, 4, 0,
	7, 8, 4, 7, 5, 1, 2, 0, 7, 7,
};

__weak void ili9335_init_gpios(void)
{
}

static int ili9335_xfer(struct spi_slave *slave, int bitlen)
{
	int ret;

	ret = spi_xfer(slave, bitlen, txbuf, rxbuf,
		       SPI_XFER_BEGIN | SPI_XFER_END);
	if (ret) {
		printf("Error %d during SPI transaction\n", ret);
		return ret;
	}

	return 0;
}

static int ili9335_txbuf(struct spi_slave *slave, int buflen)
{
	return ili9335_xfer(slave, buflen * 8);
}

static inline void convert_16bit(uchar *pbuf, ushort val)
{
	pbuf[0] = (val & 0xff00) >> 8;
	pbuf[1] = (val & 0xff);
}

static int ili9335_write_reg(struct spi_slave *slave, int len, ...)
{
	va_list args;
	int i, ret;
	ushort val;

	va_start(args, len);
	txbuf[0] = START_BYTE;

	val = va_arg(args, int);
	convert_16bit(&txbuf[1], val);
	ret = ili9335_txbuf(slave, 3);
	if (ret) {
		printf("failed to transfer data\n");
		return ret;
	}
	len--;

	txbuf[0] = START_BYTE | 0x2;

	if (len) {
		i = len;
		uchar *buf = &txbuf[1];
		while (i--) {
			val = va_arg(args, int);
			convert_16bit(buf, val);
			buf += 2;
		}
		ret = ili9335_txbuf(slave, len * 2 + 1);
		if (ret) {
			printf("failed to transfer data\n");
			return ret;
		}
	}
	va_end(args);

	return 0;
}

#define write_reg(slave, ...) \
	ili9335_write_reg(slave, NUMARGS(__VA_ARGS__), __VA_ARGS__)

static int ili9335_rxbuf(struct spi_slave *slave, int buflen)
{
	txbuf[0] |= 0x73;
	txbuf[1] = 0;
	txbuf[2] = 0;

	return ili9335_xfer(slave, buflen * 8);
}

static int ili9335_read_devicecode(struct spi_slave *slave)
{
	int ret;
	unsigned short devcode;

	ret = write_reg(slave, 0x0000);
	if (ret)
		return -1;

	ret = ili9335_rxbuf(slave, 4);
	if (ret)
		return -1;

	devcode = (rxbuf[2] << 8) | rxbuf[3];

	if ((devcode != 0x0000) && (devcode != 0x9335)) {
		printf("Devcode(0x%x) is not matched with 0x9335\n", devcode);
		return -ENODEV;
	}

	debug("device code: 0x%x\n", devcode);

	return 0;
}

static void ili9335_init_sequences(struct spi_slave *slave)
{
	write_reg(slave, 0xE5, 0x1014);
	write_reg(slave, 0x00, 0x0001);
	write_reg(slave, 0x01, 0x0100);
	write_reg(slave, 0x02, 0x0200);

	write_reg(slave, 0x08, 0x0202); /* set back & front porch */
	write_reg(slave, 0x09, 0x0000); /* set scan interval */
	write_reg(slave, 0x0a, 0x0000); /* set display control1 */
	write_reg(slave, 0x0c, 0x0000); /* set RGB I/F display control */
	write_reg(slave, 0x0d, 0x0000); /* set frame mark position */
	write_reg(slave, 0x0f, 0x0000); /* RGB Display interface control2 */

	/* power on sequence */
	write_reg(slave, 0x10, 0x0000);
	write_reg(slave, 0x11, 0x0007);
	write_reg(slave, 0x12, 0x0000);
	write_reg(slave, 0x13, 0x0000);
	mdelay(200);
	write_reg(slave, 0x10, 0x17B0);
	write_reg(slave, 0x11, 0x0031);
	mdelay(50);
	write_reg(slave, 0x12, 0x0088);
	mdelay(50);
	write_reg(slave, 0x13, 0x1800);
	write_reg(slave, 0x29, 0x0008);
	mdelay(50);
	write_reg(slave, 0x20, 0x0000);
	write_reg(slave, 0x21, 0x0000);

	write_reg(slave, 0x50, 0x0000);
	write_reg(slave, 0x51, 0x00EF);
	write_reg(slave, 0x52, 0x0000);
	write_reg(slave, 0x53, 0x013F);

	write_reg(slave, 0x60, 0x2700); /* set gate scan control */
	write_reg(slave, 0x61, 0x0000); /* Normally White 0001 for same color
					   with I2421A */
	write_reg(slave, 0x6a, 0x0000); /* set gate scan control */
	mdelay(80);

	/* partial display control */
	write_reg(slave, 0x80, 0x0000);
	write_reg(slave, 0x81, 0x0000);
	write_reg(slave, 0x82, 0x0000);
	write_reg(slave, 0x83, 0x0000);
	write_reg(slave, 0x84, 0x0000);
	write_reg(slave, 0x85, 0x0000);

	/* panel control */
	write_reg(slave, 0x90, 0x0010);
	write_reg(slave, 0x92, 0x0000);
	write_reg(slave, 0x93, 0x0003);
	write_reg(slave, 0x95, 0x0110);
	write_reg(slave, 0x97, 0x0000);
	write_reg(slave, 0x98, 0x0000);

	write_reg(slave, 0x07, 0x0173); /* display on */
	mdelay(80);
}

static void ili9335_set_var(struct spi_slave *slave)
{
	switch (ROTATE) {
	case 0:
		write_reg(slave, 0x03, (BGR << 12) | 0x30);
		break;
	case 270:
		write_reg(slave, 0x03, (BGR << 12) | 0x28);
		break;
	case 180:
		write_reg(slave, 0x03, (BGR << 12) | 0x00);
		break;
	case 90:
		write_reg(slave, 0x03, (BGR << 12) | 0x18);
		break;
	}
}

static void ili9335_set_addr_win(struct spi_slave *slave, int xs, int ys,
				 int xe, int ye)
{
	switch (ROTATE) {
	case 0:
		write_reg(slave, 0x20, xs);
		write_reg(slave, 0x21, ys);
		break;
	case 180:
		write_reg(slave, 0x20, WIDTH - 1 - xs);
		write_reg(slave, 0x21, HEIGHT - 1 - ys);
		break;
	case 270:
		write_reg(slave, 0x20, WIDTH - 1 - ys);
		write_reg(slave, 0x21, xs);
		break;
	case 90:
		write_reg(slave, 0x20, ys);
		write_reg(slave, 0x21, HEIGHT - 1 - xs);
		break;
	}

	write_reg(slave, 0x22); /* Write Data to GRAM */
}

#define CURVE(num, idx)  gamma_curves[num * GAMMA_LEN + idx]
static void ili9335_set_gamma(struct spi_slave *slave)
{
	unsigned long mask[] = {
		0x1f, 0x1f, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
		0x1f, 0x1f, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	};
	int i, j;

	/* apply mask */
	for (i = 0; i < GAMMA_NUM; i++)
		for (j = 0; j < GAMMA_LEN; j++)
			CURVE(i, j) &= mask[i * GAMMA_LEN + j];

	write_reg(slave, 0x30, CURVE(0, 5) << 8 | CURVE(0, 4));
	write_reg(slave, 0x31, CURVE(0, 7) << 8 | CURVE(0, 6));
	write_reg(slave, 0x32, CURVE(0, 9) << 8 | CURVE(0, 8));
	write_reg(slave, 0x35, CURVE(0, 3) << 8 | CURVE(0, 2));
	write_reg(slave, 0x36, CURVE(0, 1) << 8 | CURVE(0, 0));

	write_reg(slave, 0x37, CURVE(1, 5) << 8 | CURVE(1, 4));
	write_reg(slave, 0x38, CURVE(1, 7) << 8 | CURVE(1, 6));
	write_reg(slave, 0x39, CURVE(1, 9) << 8 | CURVE(1, 8));
	write_reg(slave, 0x3C, CURVE(1, 3) << 8 | CURVE(1, 2));
	write_reg(slave, 0x3D, CURVE(1, 1) << 8 | CURVE(1, 0));
}

static int ili9335_write_vmem(struct spi_slave *slave, int offset, int len)
{
	u16 *vmem16;
	size_t remain;
	size_t to_copy;
	size_t tx_array_size;
	int i;
	int ret = 0;
	size_t startbyte_size = 0;

	remain = len / 2;
	vmem16 = (u16 *)(gd->fb_base + offset);

	/* buffered write */
	tx_array_size = MAX_SPI_BYTES / 2;

	if (START_BYTE) {
		tx_array_size--;
		*(u8 *)(txbuf) = START_BYTE | 0x2;
		startbyte_size = 1;
	}

	while (remain) {
		to_copy = remain > tx_array_size ? tx_array_size : remain;

		for (i = 0; i < to_copy; i++)
			convert_16bit(&txbuf[i * 2 + 1], vmem16[i]);

		vmem16 = vmem16 + to_copy;
		ret = ili9335_txbuf(slave, startbyte_size + to_copy * 2);
		if (ret < 0)
			return ret;
		remain -= to_copy;
	}

	return 0;
}

static void ili9335_update_display(struct spi_slave *slave, unsigned start_line,
				   unsigned end_line)
{
	int line_length = WIDTH * BPP / 8;
	int offset = start_line * line_length;
	int len = (end_line - start_line + 1) * line_length;

	/* TODO: sanity check of the start_line and end_line */
	ili9335_set_addr_win(slave, 0, start_line, WIDTH - 1, end_line);

	ili9335_write_vmem(slave, offset, len);
}

static int ili9335_init_spi(void)
{
	int ret;
	unsigned int bus = CONFIG_ILI9335_SPI_BUS;
	unsigned int cs = CONFIG_ILI9335_SPI_CS;
	unsigned int max_hz = CONFIG_ILI9335_SPI_MAX_HZ;
	unsigned int mode = SPI_MODE_3;

#ifdef CONFIG_DM_SPI
	char name[30], *str;
	struct udevice *dev;

	snprintf(name, sizeof(name), "generic_%d:%d", bus, cs);
	str = strdup(name);
	ret = spi_get_bus_and_cs(bus, cs, max_hz, mode, "spi_generic_drv",
				 str, &dev, &slave);
	if (ret)
		return ret;
#else
	slave = spi_setup_slave(bus, cs, max_hz, mode);
	if (!slave) {
		printf("Invalid device %d:%d\n", bus, cs);
		return -EINVAL;
	}
#endif

	ret = spi_claim_bus(slave);
	if (ret)
		goto free_slave;

	return 0;

free_slave:
#ifndef CONFIG_DM_SPI
	spi_free_slave(slave);
#endif

	return ret;
}

static void ili9335_shutdown_spi(struct spi_slave *slave)
{
	spi_release_bus(slave);
#ifndef CONFIG_DM_SPI
	spi_free_slave(slave);
#endif
}

void lcd_ctrl_init(void *lcdbase)
{
	int ret;

	ili9335_init_gpios();

	ret = ili9335_init_spi();
	if (ret) {
		debug("%s: Failed to claim SPI bus: %d\n", __func__, ret);
		return;
	}

	/* Read Device ID */
	ret = ili9335_read_devicecode(slave);
	if (ret) {
		ili9335_shutdown_spi(slave);
		return;
	}

	ili9335_init_sequences(slave);
	ili9335_set_var(slave);

	/* Clear display */
	ili9335_update_display(slave, 0, HEIGHT - 1);
	ili9335_set_gamma(slave);
}

void lcd_enable(void)
{
}

void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blue)
{
}

void lcd_sync(void)
{
	ili9335_update_display(slave, 0, HEIGHT - 1);
}
