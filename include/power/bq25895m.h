/*
 * bq25895m.h  --  charger driver for the TI bq2578m
 *
 * Copyright (C) 2017  Nexell Co., Ltd.
 * Author: Allan, Park <allan.park@nexell.co.kr>
 *
 * SPDX-License-Identifier:     GPL-2.0+

 */

#ifndef __BQ25895M_H_
#define __BQ25895M_H_

struct dm_bq25895m_chg_platdata {
	struct gpio_desc gpio_irq;
	struct gpio_desc gpio_stat;
	int chg_vol;
	int chg_cur;
	int pre_chg_cur;
	int term_cur;
	int temp_profile;
	int safty_timer;
};

#define BQ25895M_CHG_DRIVER "bq25895m_chg"

#define REG0B	0x0B
#define	REG14	0x14

#endif /* __BQ25895M_H_ */
