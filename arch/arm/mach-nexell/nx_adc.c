/*
 * (C) Copyright 2016 Nexell
 * Seoji, Kim <seoji@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <common.h>
#include <linux/types.h>
#include <asm/io.h>
#include <adc.h>

#define MAX_ADC_SAMPELING 5
#define ADC_THRESHOLD 2047

int get_nexell_adc_val(int channel)
{
	volatile int i;
	unsigned int adcval;

	/* pclk: 200Mhz prescaler: 166 -> 1 clk (0.8us) */
	/* To read valid data, mimimum 6 clocks are needed */
	udelay(5);

	for(i = 0; i < MAX_ADC_SAMPELING; i++) {

		if (adc_channel_single_shot("adc", channel, &adcval))
			adcval = 0;

		if ((i > 0) && (adcval > ADC_THRESHOLD))
			return adcval;

		/* 5 cycles are needed per conversion */
		udelay(4);
	}
	return adcval;
}
