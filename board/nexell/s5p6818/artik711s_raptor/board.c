/*
 * (C) Copyright 2018 Samsung Electronics
 * Chanho Park <chanho61.park@samsung.com>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <asm/io.h>

#include <asm/arch/nexell.h>
#include <asm/arch/nx_gpio.h>
#include <memalign.h>

#ifdef CONFIG_DM_PMIC_NXE2000
#include <dm.h>
#include <dm/uclass-internal.h>
#include <power/pmic.h>
#include <power/nxe2000.h>
#endif

#ifdef CONFIG_USB_GADGET
#include <usb.h>
#endif

#ifdef CONFIG_ARTIK_OTA
#include <artik_ota.h>
#endif

#ifdef CONFIG_ARTIK_MAC
#include <artik_mac.h>
#endif

#ifdef CONFIG_RECOVERY_BOOT
#include <recoveryboot.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_REVISION_TAG
u32 board_rev;

u32 get_board_rev(void)
{
	return board_rev;
}

static void check_hw_revision(void)
{
	u32 val = 0;

	val |= nx_gpio_get_input_value(2, 1);
	val <<= 1;

	val |= nx_gpio_get_input_value(2, 0);
	val <<= 1;

	val |= nx_gpio_get_input_value(2, 2);

	board_rev = val;
}

static void set_board_rev(u32 revision)
{
	char info[64] = {0, };

	snprintf(info, ARRAY_SIZE(info), "%d", revision);
	setenv("board_rev", info);
}
#endif

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	printf("\nBoard: ARTIK711S Raptor\n");

	return 0;
}
#endif

#ifdef CONFIG_SET_DFU_ALT_INFO
void set_dfu_alt_info(char *interface, char *devstr)
{
	size_t buf_size = CONFIG_SET_DFU_ALT_BUF_LEN;
	ALLOC_CACHE_ALIGN_BUFFER(char, buf, buf_size);

	snprintf(buf, buf_size, "setenv dfu_alt_info \"%s\"", CONFIG_DFU_ALT);
	run_command(buf, 0);
}
#endif

/*------------------------------------------------------------------------------
 * intialize nexell soc and board status.
 */

static void nx_phy_init(void)
{
}

/* call from u-boot */
int board_early_init_f(void)
{
	return 0;
}

int mmc_get_env_dev(void)
{
	int port_num;
	int boot_mode = readl(PHY_BASEADDR_CLKPWR + SYSRSTCONFIG);

	if ((boot_mode & BOOTMODE_MASK) == BOOTMODE_SDMMC ||
	    (boot_mode & BOOTMODE_MASK) == BOOTMODE_USB) {
		port_num = readl(SCR_ARM_SECOND_BOOT_REG1);

		/* Reverse order compared with artik710 */
		if (port_num == SD_PORT_NUM)
			return 0;
		else if (port_num == EMMC_PORT_NUM)
			return 1;
	}

	return -1;
}

int board_init(void)
{
#ifdef CONFIG_REVISION_TAG
	check_hw_revision();
	printf("HW Revision:\t%d\n", board_rev);

#endif

	nx_phy_init();

	return 0;
}

/* u-boot dram initialize  */
int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;
	return 0;
}

/* u-boot dram board specific */
void dram_init_banksize(void)
{
	/* set global data memory */
	gd->bd->bi_arch_number = machine_arch_type;
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x00000100;

	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size  = CONFIG_SYS_SDRAM_SIZE;
}

#ifdef CONFIG_DM_PMIC_NXE2000
void pmic_init(void)
{
	static struct udevice *dev;
	int ret = -ENODEV;
	uint8_t bit_mask = 0;

	ret = pmic_get("nxe2000_gpio@32", &dev);
	if (ret) {
		printf("Can't get PMIC: %s!\n", "nxe2000_gpio@32");
		return;
	}

	bit_mask = pmic_reg_read(dev, NXE2000_REG_PWRONTIMSET);
	bit_mask &= ~(0x1 << NXE2000_POS_PWRONTIMSET_OFF_JUDGE_PWRON);
	bit_mask |= (0x0 << NXE2000_POS_PWRONTIMSET_OFF_JUDGE_PWRON);
	ret = pmic_write(dev, NXE2000_REG_PWRONTIMSET, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC REG: %d!\n", NXE2000_REG_PWRONTIMSET);

	bit_mask = 0x00;
	ret = pmic_reg_write(dev, (u32)NXE2000_REG_BANKSEL, (u32)bit_mask);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_BANKSEL);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(3 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(0 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC1CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC1CTL2);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(3 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(0 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC2CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC2CTL2);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC3CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC3CTL2);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC4CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC4CTL2);

	bit_mask = ((0 << NXE2000_POS_DCxCTL2_DCxOSC) |
			(0 << NXE2000_POS_DCxCTL2_DCxSR) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIM) |
			(1 << NXE2000_POS_DCxCTL2_DCxLIMSDEN));
	ret = pmic_write(dev, NXE2000_REG_DC5CTL2, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_DC5CTL2);

	bit_mask = (1 << NXE2000_POS_CHGCTL1_SUSPEND);
	ret = pmic_write(dev, NXE2000_REG_CHGCTL1, &bit_mask, 1);
	if (ret)
		printf("Can't write PMIC register: %d!\n", NXE2000_REG_CHGCTL1);
}
#endif

int board_late_init(void)
{
#ifdef CONFIG_DM_PMIC_NXE2000
	pmic_init();
#endif
#ifdef CONFIG_REVISION_TAG
	set_board_rev(board_rev);
#endif
#ifdef CONFIG_CMD_FACTORY_INFO
	run_command("run factory_load", 0);
#endif
#ifdef CONFIG_ARTIK_MAC
	generate_mac();
#endif
#ifdef CONFIG_ARTIK_OTA
	check_ota_update();
#endif
#ifdef CONFIG_RECOVERY_BOOT
	if (check_recoveryboot())
		setenv("bootcmd", "run recoveryboot");
#endif
	return 0;
}

#ifdef CONFIG_USB_GADGET
int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	if (!strcmp(name, "usb_dnl_thor")) {
		put_unaligned(CONFIG_G_DNL_THOR_VENDOR_NUM, &dev->idVendor);
		put_unaligned(CONFIG_G_DNL_THOR_PRODUCT_NUM, &dev->idProduct);
	} else if (!strcmp(name, "usb_dnl_ums")) {
		put_unaligned(CONFIG_G_DNL_UMS_VENDOR_NUM, &dev->idVendor);
		put_unaligned(CONFIG_G_DNL_UMS_PRODUCT_NUM, &dev->idProduct);
	} else {
		put_unaligned(CONFIG_G_DNL_VENDOR_NUM, &dev->idVendor);
		put_unaligned(CONFIG_G_DNL_PRODUCT_NUM, &dev->idProduct);
	}
	return 0;
}
#endif
