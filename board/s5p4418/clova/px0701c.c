/*
 * Copyright (C) 2017  Nexell Co., Ltd.
 *
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <linux/compat.h>
#include <linux/err.h>
#include <asm/gpio.h>
#include <asm/arch/mipi_display.h>

#define PX0701C_WIDTH_MM	94.2
#define PX0701C_HEIGHT_MM	150.72

#define RESET_GPIO		21	/* GPA21 */
#define RESET_DELAY		1	/* ms */
#define POWER_ON_DELAY		1	/* ms */
#define INIT_DELAY		200	/* ms */
#define FLIP_HORIZONTAL		0
#define FLIP_VERTICAL		0

#define mdelay(a)	udelay(a * 1000)

struct px0701c {
	struct mipi_dsi_device *dsi;
	int reset_gpio;
	u32 reset_delay;
	u32 power_on_delay;
	u32 init_delay;
	bool flip_horizontal;
	bool flip_vertical;
	bool is_power_on;
};

static inline struct px0701c *dsi_to_px0701c(struct mipi_dsi_device *dsi)
{
	return dsi->ops->private_data;
}

static int px0701c_power_on(struct px0701c *ctx)
{
	if (ctx->is_power_on)
		return 0;

	mdelay(ctx->power_on_delay);

	gpio_direction_output(ctx->reset_gpio, 1);
	gpio_set_value(ctx->reset_gpio, 0);
	mdelay(ctx->reset_delay);
	gpio_set_value(ctx->reset_gpio, 1);
	mdelay(ctx->init_delay);

	ctx->is_power_on = true;

	return 0;
}

static int px0701c_power_off(struct px0701c *ctx)
{
	if (!ctx->is_power_on)
		return 0;

	gpio_set_value(ctx->reset_gpio, 0);
	ctx->is_power_on = false;

	return 0;
}

static int px0701c_prepare(struct mipi_dsi_device *dsi)
{
	struct px0701c *ctx = dsi_to_px0701c(dsi);

	return px0701c_power_on(ctx);
}

static int px0701c_unprepare(struct mipi_dsi_device *dsi)
{
	struct px0701c *ctx = dsi_to_px0701c(dsi);

	return px0701c_power_off(ctx);
}

static int px0701c_enable(struct mipi_dsi_device *dsi)
{
	return 0;
}

static int px0701c_disable(struct mipi_dsi_device *dsi)
{
	return 0;
}

static struct mipi_panel_ops px0701c_ops = {
	.prepare = px0701c_prepare,
	.unprepare = px0701c_unprepare,
	.enable = px0701c_enable,
	.disable = px0701c_disable,
};

static int px0701c_parse_dt(struct px0701c *ctx)
{
	ctx->reset_gpio = RESET_GPIO;
	ctx->reset_delay = RESET_DELAY;
	ctx->power_on_delay = POWER_ON_DELAY;
	ctx->init_delay = INIT_DELAY;
	ctx->flip_horizontal = FLIP_HORIZONTAL;
	ctx->flip_vertical = FLIP_VERTICAL;

	return 0;
}

static int px0701c_init(struct mipi_dsi_device *dsi)
{
	struct px0701c *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->dsi = dsi;
	ctx->is_power_on = false;

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO |
		MIPI_DSI_MODE_VIDEO_HFP | MIPI_DSI_MODE_VIDEO_HBP |
		MIPI_DSI_MODE_VIDEO_HSA | MIPI_DSI_MODE_VSYNC_FLUSH;
	dsi->ops = &px0701c_ops;
	dsi->ops->private_data = ctx;

	ret = px0701c_parse_dt(ctx);
	if (ret < 0)
		return ret;

	ret = gpio_request(ctx->reset_gpio, "reset-gpio");
	if (ret) {
		printf("fail: px071x request reset-gpio of %d\n", ctx->reset_gpio);
		return ret;
	}

	return 0;
}

int nx_mipi_dsi_lcd_bind(struct mipi_dsi_device *dsi)
{
	return px0701c_init(dsi);
}
