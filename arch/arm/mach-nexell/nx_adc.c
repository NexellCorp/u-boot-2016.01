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

int get_nexell_adc_val(int channel)
{
	volatile int i;
	unsigned int adcval;

	for(i = 0;i < 100; i++) {
		udelay(1);
		if (adc_channel_single_shot("adc", channel, &adcval))
			adcval = 0;
		if (i < 4) {
		} else {
			/* high */
			if (adcval > 3000) {
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
