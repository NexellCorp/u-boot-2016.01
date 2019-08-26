/*
 * Copyright 2008 - 2009 Windriver, <www.windriver.com>
 * Author: Tom Rix <Tom.Rix@windriver.com>
 *
 * (C) Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <console.h>
#include <g_dnl.h>
#include <usb.h>

#ifdef CONFIG_ARCH_NEXELL
static bool bind_nexell;

int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	u16 vid, pid;

	get_usbid(&vid, &pid);
	debug("%s %x %x\n", __func__, vid, pid);

	if (bind_nexell == true) {
		put_unaligned(vid, &dev->idVendor);
		put_unaligned(pid, &dev->idProduct);
	} else {
		put_unaligned(__constant_cpu_to_le16(CONFIG_G_DNL_VENDOR_NUM),
			&dev->idVendor);
		put_unaligned(__constant_cpu_to_le16(CONFIG_G_DNL_PRODUCT_NUM),
			&dev->idProduct);
	}

	return 0;
}
#endif

static int do_fastboot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int controller_index;
	char *usb_controller;
	int ret;

#ifdef CONFIG_ARCH_NEXELL
	bind_nexell = false;
	if (argc == 3) {
		if (!strcmp(argv[2], "nexell"))
			 bind_nexell = true;
	}
#else
	if (argc < 2)
		return CMD_RET_USAGE;
#endif

#if defined(CONFIG_ARCH_S5P6818) && !defined(CONFIG_SYS_DCACHE_OFF)
	dcache_disable();
#endif

	usb_controller = argv[1];
	controller_index = simple_strtoul(usb_controller, NULL, 0);

	ret = board_usb_init(controller_index, USB_INIT_DEVICE);
	if (ret) {
		error("USB init failed: %d", ret);
		return CMD_RET_FAILURE;
	}

	g_dnl_clear_detach();
	ret = g_dnl_register("usb_dnl_fastboot");
	if (ret)
		return ret;

	if (!g_dnl_board_usb_cable_connected()) {
		puts("\rUSB cable not detected.\n" \
		     "Command exit.\n");
		ret = CMD_RET_FAILURE;
		goto exit;
	}

#ifdef CONFIG_ARCH_NEXELL
	printf("FASTBOOT [%s] Wait for download ...\n", bind_nexell ?
		   "Nexell" : "Google");
#endif
	while (1) {
		if (g_dnl_detach())
			break;
		if (ctrlc())
			break;
		usb_gadget_handle_interrupts(controller_index);
	}

	ret = CMD_RET_SUCCESS;

exit:
	g_dnl_unregister();
	g_dnl_clear_detach();
	board_usb_cleanup(controller_index, USB_INIT_DEVICE);

	return ret;
}

U_BOOT_CMD(
#ifdef CONFIG_ARCH_NEXELL
	fastboot, 3, 1, do_fastboot,
	"use USB Fastboot protocol",
	"<USB_controller> <nexell>\n"
	"	 - run as a fastboot usb device\n"
	"	 - The argument 'nexell' is optional for Nexell USBID"
#else
	fastboot, 2, 1, do_fastboot,
	"use USB Fastboot protocol",
	"<USB_controller>\n"
	"    - run as a fastboot usb device"
#endif
);
