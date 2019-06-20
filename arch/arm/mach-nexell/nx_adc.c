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

#define MAX_ADC_SIZE 15
#define MAX_ADC_SAMPELING 100
#define ADC_THRESHOLD 2047

int get_nexell_adc_val(int channel)
{
	volatile int i;
	unsigned int adcval;

	for(i = 0; i < MAX_ADC_SAMPELING; i++) {
		udelay(1);
		if (adc_channel_single_shot("adc", channel, &adcval))
			adcval = 0;
		if (i >= 4) {
			/* high */
			if (adcval > ADC_THRESHOLD) {
				break;
			/* low */
			} else {
				if (i >= MAX_ADC_SIZE)
					break;
			}
		}
	}
	return adcval;
}
