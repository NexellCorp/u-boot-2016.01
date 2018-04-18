/*
 * Copyright (C) 2018  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <recoveryboot.h>

#define ALIVE_SCRATCH1_READ_REGISTER    (0xc00108b4)
#define ALIVE_SCRATCH1_RESET_REGISTER   (0xc00108ac)
/* (ASCII) : R.E.S.C */
#define RECOVERY_SIGNATURE              (0x52455343)

int check_recoveryboot(void)
{
	if (readl(ALIVE_SCRATCH1_READ_REGISTER) == RECOVERY_SIGNATURE) {
		printf("reboot recovery\n");
		writel(0xffffffff, ALIVE_SCRATCH1_RESET_REGISTER);
		return 1;
	}

	return 0;
}
