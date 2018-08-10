#ifndef _ARM_SERIAL_MCU_H
#define _ARM_SERIAL_MCU_H
/*
 * arm_serial_mcu.h
 *
 * Copyright (C) 2018  ZhongHong Co., Ltd.
 * Author: 
 *
 * SPDX-License-Identifier:     GPL-2.0+

 */


#include <linux/types.h>

void mcu_informed_uart_arm_start(void);
void mcu_informed_uart_arm_late_start(void);
int mcu_serial_is_recovery(void);
void mcu_serial_do_recovery(void);
void mcu_get_max9286(void);

#endif	/* _ARM_SERIAL_MCU_H */
