/*
 * Copyright (c) 2019 Nexell
 * Author: Sungwoo Park <swpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/nexell.h>
#include <nexell_recovery.h>

#ifdef CONFIG_AUTORECOVERY_CMD
void check_nexell_autorecovery(void)
{
	char *autorecovery_cmd;

	autorecovery_cmd = getenv("autorecovery_cmd");
	if (strncmp(autorecovery_cmd, "none", 4)) {
		printf("autorecovery_cmd --> %s\n", autorecovery_cmd);
		run_command(autorecovery_cmd, 0);
	}
}

__weak int nexell_mmc_get_env_dev(void)
{
	int port_num;
	int boot_mode = readl(PHY_BASEADDR_CLKPWR + SYSRSTCONFIG);

	if ((boot_mode & BOOTMODE_MASK) == BOOTMODE_SDMMC) {
		port_num = readl(SCR_ARM_SECOND_BOOT_REG1);

		if (port_num == EMMC_PORT_NUM)
			return 0;
		else if (port_num == SD_PORT_NUM)
			return 1;
	} else if ((boot_mode & BOOTMODE_MASK) == BOOTMODE_USB) {
		return 0;
	}

	return 0;
}

int mmc_get_env_dev(void)
{
	return nexell_mmc_get_env_dev();
}
#endif
