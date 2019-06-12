/*
 * (C) Copyright 2016 Nexell
 * Seoji, Kim <seoji@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <dm.h>
#include <adc.h>
#include <asm/arch/nx_adc.h>
#include <asm/io.h>
#include <asm/arch/reset.h>
#include <asm/arch/clk.h>
#include <asm/arch/nexell.h>

struct nexell_adc_priv {
	int active_channel;
	struct s5p4418_adc *regs;
};

int nexell_adc_channel_data(struct udevice *dev, int channel,
			    unsigned int *data)
{
	struct nexell_adc_priv *priv = dev_get_priv(dev);
	struct s5p4418_adc *regs = priv->regs;
	if (channel != priv->active_channel) {
		error("Requested channel is not active!");
		return -EINVAL;
	}

	*data = readl(&regs->dat) & ADC_DAT_MASK;

	return 0;
}

int nexell_adc_start_channel(struct udevice *dev, int channel)
{
	struct nexell_adc_priv *priv = dev_get_priv(dev);
	struct s5p4418_adc *regs = priv->regs;
	unsigned int cfg;

	/* Choose channel */
	cfg = readl(&regs->con);
	cfg &= ~ADC_CHAN_SEL_MASK;
	cfg |= ADC_CHAN_SEL(channel);
	writel(cfg, &regs->con);

	/* Enable INT */
	writel(0x1, &regs->intenb);

	/* ADC Enable */
	/* Start conversion */
	cfg = readl(&regs->con);
	writel(cfg | ADC_CON_EN, &regs->con);

	priv->active_channel = channel;

	return 0;
}

int nexell_adc_stop(struct udevice *dev)
{
	struct nexell_adc_priv *priv = dev_get_priv(dev);
	struct s5p4418_adc *regs = priv->regs;
	unsigned int cfg;

	/* Stop conversion */
	cfg = readl(&regs->con);
	cfg |= ~ADC_CON_EN;

	writel(cfg, &regs->con);

	priv->active_channel = -1;

	return 0;
}

int nexell_adc_probe(struct udevice *dev)
{
	struct nexell_adc_priv *priv = dev_get_priv(dev);
	struct s5p4418_adc *regs = priv->regs;
	unsigned int cfg;

	/* reset - to read memory */
	nx_rstcon_setrst(RESET_ID_ADC, RSTCON_ASSERT);
	udelay(10);
	nx_rstcon_setrst(RESET_ID_ADC, RSTCON_NEGATE);

	/* disable adc (APEN) */
	cfg = readl(&regs->con);
	cfg &= ~ADC_CON_APEN;
	writel(cfg, &regs->con);

	/* Disable INT */
	writel(0x0, &regs->intenb);

	/* INT clear  */
	writel(0x0, &regs->intclr);

	/* STBY on */
	cfg = readl(&regs->con);
	cfg &= ~ADC_STBY_MASK;
	writel(cfg, &regs->con);

	/* CLKIN Divide val(APSV) default ff */
	cfg = readl(&regs->con);
	cfg &= ~ADC_APSV_MASK;
	cfg |= 0xA6 << 6;
	writel(cfg, &regs->con);

	/* CLKIN On(APEN) */
	cfg = readl(&regs->con);
	cfg |= ADC_CON_APEN;
	writel(cfg, &regs->con);

	cfg = readl(&regs->con);

	return 0;
}

/* converted from device tree */
int nexell_adc_ofdata_to_platdata(struct udevice *dev)
{
	struct adc_uclass_platdata *uc_pdata = dev_get_uclass_platdata(dev);
	struct nexell_adc_priv *priv = dev_get_priv(dev);

	priv->regs = (struct s5p4418_adc *)dev_get_addr(dev);
	if (priv->regs == (struct s5p4418_adc *)FDT_ADDR_T_NONE) {
		error("Dev: %s - can't get address!", dev->name);
		return -ENODATA;
	}

	uc_pdata->data_mask = ADC_DAT_MASK;
	uc_pdata->data_format = ADC_DATA_FORMAT_BIN;
	uc_pdata->data_timeout_us = ADC_CONV_TIMEOUT_US;

	/* Mask available channel bits: [0:7] */
	uc_pdata->channel_mask = (2 << ADC_MAX_CHANNEL) - 1;

	return 0;
}

static const struct adc_ops nexell_adc_ops = {
	.start_channel = nexell_adc_start_channel,
	.channel_data = nexell_adc_channel_data,
	.stop = nexell_adc_stop,
};

static const struct udevice_id nexell_adc_ids[] = {
	{ .compatible = "nexell,s5p4418-adc" },
	{ }
};

U_BOOT_DRIVER(nexell_adc) = {
	.name		= "s5p4418-adc",
	.id		= UCLASS_ADC,
	.of_match	= nexell_adc_ids,
	.ops		= &nexell_adc_ops,
	.probe		= nexell_adc_probe,
	.ofdata_to_platdata = nexell_adc_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct nexell_adc_priv),
};
