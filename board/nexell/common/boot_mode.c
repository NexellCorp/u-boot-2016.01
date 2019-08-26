// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2019 Nexell
 * Junghyun, Kim <mingyoungbo@nexell.co.kr>
 *
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>
#include <linux/err.h>

extern int is_usb_bootmode(void);

int run_fastboot_update(void)
{
#ifdef CONFIG_CMD_FASTBOOT
	if (is_usb_bootmode() == true) {
		setenv("bootcmd", "fastboot 0 nexell");
		return run_command("fastboot 0 nexell", 0);
	}
#endif
	return 0;
}
