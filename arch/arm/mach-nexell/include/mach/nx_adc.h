/*
 *  Copyright (C) 2010 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *  MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __NX_ADC_H__
#define __NX_ADC_H__

#define ADC_MAX_CHANNEL		8

/* For default 8 time convertion with sample rate 600 kSPS - 15us timeout */
#define ADC_CONV_TIMEOUT_US		15

#define ADC_CON_EN		0x1
#define ADC_CON_APEN	0x1 << 14
#define ADC_STBY_MASK	0x1 << 2
#define ADC_DAT_MASK	0xfff
#define ADC_CHAN_SEL_MASK 0x7 << 3
#define ADC_CHAN_SEL(x)	(((x) & 0x7) << 3)
#define ADC_ASEL_MASK 0x7 << 3
#define ADC_APSV_MASK 0xff << 6


#ifndef __ASSEMBLY__
struct s5p4418_adc {
	unsigned int con;
	unsigned int dat;
	unsigned int intenb;
	unsigned int intclr;
};
#endif

int nexell_adc_channel_data(struct udevice *dev, int channel, unsigned int *data);
int nexell_adc_start_channel(struct udevice *dev, int channel);
int nexell_adc_stop(struct udevice *dev);
int nexell_adc_probe(struct udevice *dev);
int nexell_adc_ofdata_to_platdata(struct udevice *dev);

#endif /* __NX_ADC_H__ */
