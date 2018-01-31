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
    unsigned int ichg;    /* charge current       */
    unsigned int vreg;    /* regulation voltage       */
    unsigned int iterm;   /* termination current      */
    unsigned int iprechg; /* precharge current        */
    unsigned int sysvmin; /* minimum system voltage limit */
    unsigned int boostv;  /* boost regulation voltage */
    unsigned int boosti;  /* boost current limit      */
    unsigned int boostf;  /* boost frequency      */
    unsigned int ilim_en; /* enable ILIM pin      */
    unsigned int treg;    /* thermal regulation threshold */
};

#define BQ25895M_CHG_DRIVER "bq25895m_chg"

#define REG0B	0x0B
#define	REG14	0x14

#endif /* __BQ25895M_H_ */
