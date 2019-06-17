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
void quick_sort(unsigned int *data, int start, int end)
{
	int pivot = start;
	int i = pivot + 1;
	int j = end;
	int temp;

	if (start >= end) {
		return;
	}

	while (i <= j) {
		while (i <= end && data[i] <= data[pivot]) {
			i++;
		}
		while (j > start && data[j] >= data[pivot]) {
			j--;
		}

		if (i > j) {
			temp = data[j];
			data[j] = data[pivot];
			data[pivot] = temp;
		} else {
			temp = data[i];
			data[i] = data[j];
			data[j] = temp;
		}
	}

	quick_sort(data, start, j - 1);
	quick_sort(data, j + 1, end);
}

int get_nexell_adc_val(int channel)
{
	volatile int i;
	int start = 5;
	int end = MAX_ADC_SIZE -1;
	unsigned int adcval[MAX_ADC_SIZE];

	for (i = 0; i < MAX_ADC_SIZE; i++) {
		udelay(1);
		if (adc_channel_single_shot("adc", channel, &adcval[i]))
			*(adcval+i) = 0;
	}

	/* quick sort */
	quick_sort(adcval, start, end);

	return adcval[(start + end) / 2];
}
