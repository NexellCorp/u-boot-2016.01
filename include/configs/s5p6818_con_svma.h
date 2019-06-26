/*
 * (C) Copyright 2016 Nexell
 * Sungwoo Park <swpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <linux/sizes.h>

/*-----------------------------------------------------------------------
 *  u-boot-2016.01
 */
#define CONFIG_SYS_LDSCRIPT "arch/arm/cpu/armv8/u-boot.lds"

#define CONFIG_MACH_S5P6818

/*-----------------------------------------------------------------------
 *  System memory Configuration
 */

#define CONFIG_SYS_TEXT_BASE			0x43C00000
#define CONFIG_SYS_INIT_SP_ADDR			CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_MONITOR_BASE			CONFIG_SYS_TEXT_BASE

#define CONFIG_SYS_MEM_SIZE			0x80000000
#define CONFIG_SYS_RESERVE_MEM_SIZE		0x02500000 /* 37MB */
#define CONFIG_SYS_SDRAM_BASE			0x40000000
#define CONFIG_SYS_SDRAM_SIZE			CONFIG_SYS_MEM_SIZE - \
						CONFIG_SYS_RESERVE_MEM_SIZE

#define CONFIG_SYS_MALLOC_LEN			(32*1024*1024)

/* fastboot buffer start, size */
#define CONFIG_FASTBOOT_BUF_ADDR		CONFIG_SYS_SDRAM_BASE
#define CONFIG_FASTBOOT_BUF_SIZE		0x38000000
/* align buffer is used by ext4_mmc_write for unaligned data */
#define CONFIG_ALIGNBUFFER_SIZE			0x02000000

/* when CONFIG_LCD */
#define CONFIG_FB_ADDR				(CONFIG_FASTBOOT_BUF_ADDR + \
						 CONFIG_FASTBOOT_BUF_SIZE + \
						 CONFIG_ALIGNBUFFER_SIZE)

/* dram 1 bank num */
#define CONFIG_NR_DRAM_BANKS			1

/* kernel load address */
#define CONFIG_SYS_LOAD_ADDR			0x48000000

/* AARCH64 */
#define COUNTER_FREQUENCY			200000000
#define CPU_RELEASE_ADDR			CONFIG_SYS_INIT_SP_ADDR

/* memtest works on */
#define CONFIG_SYS_MEMTEST_START		CONFIG_SYS_SDRAM_BASE
#define CONFIG_SYS_MEMTEST_END			((ulong)CONFIG_SYS_SDRAM_BASE \
						 + (ulong)CONFIG_SYS_SDRAM_SIZE)
/*-----------------------------------------------------------------------
 *  High Level System Configuration
 */

/* Not used: not need IRQ/FIQ stuff	*/
#undef  CONFIG_USE_IRQ
/* decrementer freq: 1ms ticks */
#define CONFIG_SYS_HZ				1000

/*-----------------------------------------------------------------------
 *  System initialize options (board_init_f)
 */

/* board_init_f->init_sequence, call arch_cpu_init */
#define CONFIG_ARCH_CPU_INIT

#define CONFIG_BOARD_LATE_INIT
/* board_init_f->init_sequence, call print_cpuinfo */
#define CONFIG_DISPLAY_CPUINFO
/* board_init_f, CONFIG_SYS_ICACHE_OFF */
/* #define	CONFIG_SYS_DCACHE_OFF */
/* board_init_r, call arch_misc_init */
#define CONFIG_ARCH_MISC_INIT
/*#define	CONFIG_SYS_ICACHE_OFF*/

/*-----------------------------------------------------------------------
 *	U-Boot default cmd
 */
#define CONFIG_CMD_MEMTEST

/*-----------------------------------------------------------------------
 *	U-Boot Environments
 */
/* refer to common/env_common.c	*/
#define CONFIG_BOOTDELAY			0
#define CONFIG_ZERO_BOOTDELAY_CHECK

/*-----------------------------------------------------------------------
 * Miscellaneous configurable options
 */
#ifdef CONFIG_SYS_PROMPT
#undef CONFIG_SYS_PROMPT
/* Monitor Command Prompt   */
#define CONFIG_SYS_PROMPT			"nxp5430_con_svma# "
#endif
/* undef to save memory	   */
#define CONFIG_SYS_LONGHELP
/* Console I/O Buffer Size  */
#define CONFIG_SYS_CBSIZE			1024
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE			(CONFIG_SYS_CBSIZE + \
						 sizeof(CONFIG_SYS_PROMPT)+16)
/* max number of command args   */
#define CONFIG_SYS_MAXARGS			16
/* Boot Argument Buffer Size    */
#define CONFIG_SYS_BARGSIZE			CONFIG_SYS_CBSIZE

/*-----------------------------------------------------------------------
 * allow to overwrite serial and ethaddr
 */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_SYS_HUSH_PARSER			/* use "hush" command parser */
#ifdef CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#endif

/*-----------------------------------------------------------------------
 * Etc Command definition
 */
#define CONFIG_CMD_IMI				/* image info	*/
#define CONFIG_CMDLINE_EDITING			/* add command line history */
#define CONFIG_CMDLINE_TAG			/* use bootargs commandline */
#define CONFIG_INITRD_TAG
#undef CONFIG_BOOTM_NETBSD
#undef CONFIG_BOOTM_RTEMS

/*-----------------------------------------------------------------------
 * serial console configuration
 */
#define CONFIG_S5P_SERIAL
#define CONFIG_S5P_SERIAL_INDEX     0
#define CONFIG_S5P_SERIAL_CLOCK     50000000
#define CONFIG_S5P_SERIAL_PORT      (void *)PHY_BASEADDR_UART0

#define CONFIG_BAUDRATE				115200
#define CONFIG_SYS_BAUDRATE_TABLE \
		{ 9600, 19200, 38400, 57600, 115200 }
#define CONFIG_S5P_SERIAL_FLUSH_ON_INIT

#define CONFIG_UART_CLKGEN_CLOCK_HZ	CONFIG_S5P_SERIAL_CLOCK

/*-----------------------------------------------------------------------
 * NO FLASH
 */
#define CONFIG_SYS_NO_FLASH

/*-----------------------------------------------------------------------
 * PLL
 */
#define CONFIG_SYS_PLLFIN		24000000UL

/*-----------------------------------------------------------------------
 * Timer
 */
#define CONFIG_TIMER_SYS_TICK_CH	0

/*-----------------------------------------------------------------------
 * PWM
 */
#define CONFIG_PWM_NX

/*-----------------------------------------------------------------------
 * BACKLIGHT Primary(RGBtoDualLVDS)
 */
#define CONFIG_BACKLIGHT_CH		0
#define CONFIG_BACKLIGHT_DIV		0
#define CONFIG_BACKLIGHT_INV		0
#define CONFIG_BACKLIGHT_DUTY		50
#define CONFIG_BACKLIGHT_HZ		40000

/*-----------------------------------------------------------------------
 * BACKLIGHT Secondary(LVDS)
 */
#define CONFIG_SBACKLIGHT_CH		3
#define CONFIG_SBACKLIGHT_DIV		0
#define CONFIG_SBACKLIGHT_INV		0
#define CONFIG_SBACKLIGHT_DUTY		50
#define CONFIG_SBACKLIGHT_HZ		100000

/*-----------------------------------------------------------------------
 * SD/MMC
 */
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_DWMMC
#define CONFIG_NEXELL_DWMMC
#define CONFIG_BOUNCE_BUFFER
#define CONFIG_CMD_MMC

#if defined(CONFIG_MMC)
#define CONFIG_2NDBOOT_OFFSET		512
#define CONFIG_2NDBOOT_SIZE		(64*1024)
#define CONFIG_FIP_OFFSET		(CONFIG_2NDBOOT_OFFSET +\
					 CONFIG_2NDBOOT_SIZE)
#define CONFIG_FIP_SIZE			    (2880*1024)
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_ENV_OFFSET		(CONFIG_FIP_OFFSET +\
					 CONFIG_FIP_SIZE)
#define CONFIG_ENV_SIZE			(16*1024)

#define CONFIG_DOS_PARTITION
#define CONFIG_CMD_FAT
#define CONFIG_FS_FAT
#define CONFIG_FAT_WRITE

#define CONFIG_CMD_EXT4
#define CONFIG_CMD_EXT4_WRITE
#define CONFIG_FS_EXT4
#define CONFIG_EXT4_WRITE
#endif

/*-----------------------------------------------------------------------
 * Default environment organization
 */
#if !defined(CONFIG_ENV_IS_IN_MMC) && !defined(CONFIG_ENV_IS_IN_NAND) && \
	!defined(CONFIG_ENV_IS_IN_FLASH) && !defined(CONFIG_ENV_IS_IN_EEPROM)
	/* default: CONFIG_ENV_IS_NOWHERE */
	#define CONFIG_ENV_IS_NOWHERE
	#define CONFIG_ENV_OFFSET		1024
	#define CONFIG_ENV_SIZE			(4*1024)	/* env size */
	/* imls - list all images found in flash, default enable so disable */
	#undef CONFIG_CMD_IMLS
#endif

/*-----------------------------------------------------------------------
 * GPT
 */
#define CONFIG_CMD_GPT
#define CONFIG_EFI_PARTITION
#define CONFIG_PARTITION_UUIDS
#define CONFIG_RANDOM_UUID

/*-----------------------------------------------------------------------
 * Fastboot and USB OTG
 */
#define CONFIG_USB_FUNCTION_FASTBOOT
#define CONFIG_CMD_FASTBOOT
#define CONFIG_FASTBOOT_FLASH
#define CONFIG_FASTBOOT_FLASH_MMC_DEV		0
#define CONFIG_USB_GADGET
#define CONFIG_USB_GADGET_DUALSPEED
#define CONFIG_USB_GADGET_VBUS_DRAW		0
#define CONFIG_USB_GADGET_DWC2_OTG
#define CONFIG_USB_GADGET_NX_UDC_OTG_PHY
#define CONFIG_USB_GADGET_DOWNLOAD
#define CONFIG_SYS_CACHELINE_SIZE		64
#define CONFIG_G_DNL_VENDOR_NUM			0x18d1  /* google */
#define CONFIG_G_DNL_PRODUCT_NUM		0x0002  /* nexus one */
#define CONFIG_G_DNL_MANUFACTURER		"Nexell Corporation"

/*-----------------------------------------------------------------------
 * Nexell USB Downloader
 */
#define CONFIG_NX_USBDOWN

/*-----------------------------------------------------------------------
 * OF_CONTROL
 */
#define CONFIG_FIT_BEST_MATCH
#define CONFIG_OF_LIBFDT

/*-----------------------------------------------------------------------
 * VIDEO
 */
#define CONFIG_VIDEO
#define CONFIG_CFB_CONSOLE
#define CONFIG_VGA_AS_SINGLE_DEVICE
#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#define CONFIG_VIDEO_LOGO
#define CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SCREEN_ALIGN

#ifdef CONFIG_VIDEO_LOGO
#define CONFIG_CMD_BMP
#ifdef CONFIG_SPLASH_SCREEN
#define CONFIG_SPLASH_SOURCE
#define CONFIG_SPLASH_MMC_OFFSET		0x2e4200
#endif
#endif

/*-----------------------------------------------------------------------
 * Support Android Boot Image
*/
#define CONFIG_ANDROID_BOOT_IMAGE
#define CONFIG_SUPPORT_RAW_INITRD
#define CONFIG_RECOVERY_BOOT

/* partition number infomation */
/* 1,2 - bootloader_a, bootloader_b */
/* 3,5 - boot_a, boot_b */
/* 6,7 - system_a, system_b */
/* 8,9 - vendor_a, vendor_b */
/* 10(A)   - misc */
/* 11(B)   - data */

/* #define CONTROL_PARTITION 8 //"misc" */
#define CONTROL_PARTITION A //"misc"

#if defined(CONFIG_CMD_AB_SELECT)
#define SET_AB_SELECT \
       "if ab_select slot_name mmc 0:${misc_partition_num}; " \
       "then " \
               "echo ab_select get slot_name success;" \
       "else " \
               "echo ab_select get slot_name failed, set slot \"a\";" \
               "setenv slot_name a;" \
       "fi;" \
       "setenv slot_suffix _${slot_name};" \
       "setenv android_boot_option androidboot.slot_suffix=${slot_suffix};" \
       "setenv android_boot_ab run bootcmd_${slot_name};" \
       "if test ${slot_name} = a ; " \
       "then " \
               "setenv root_dev_blk_system_ab /dev/mmcblk0p6 ;" \
       "else " \
               "setenv root_dev_blk_system_ab /dev/mmcblk0p7 ;" \
       "fi;" \
       "setenv bootargs_ab1 root=${root_dev_blk_system_ab};" \
       "setenv bootargs_ab2 ${android_boot_option};"
#else
#define SET_AB_SELECT ""
#endif //CONFIG_CMD_AB_SELECT

/*-----------------------------------------------------------------------
 * ENV
 */
#define CONFIG_REVISION_TAG 1
/* need to relocate env address */
#define CONFIG_KERNEL_DTB_ADDR	0x49000000
#define CONFIG_BMP_LOAD_ADDR	0x80000000

#define CONFIG_EXTRA_ENV_BOOT_LOGO				\
	"splashimage=" __stringify(CONFIG_BMP_LOAD_ADDR)"\0"	\
	"splashfile=logo.bmp\0"				\
	"splashsource=mmc_fs\0"				\
	"splashoffset=" __stringify(CONFIG_SPLASH_MMC_OFFSET)"\0"	\
	"splashpos=m,m\0"					\
	"fb_addr=\0"						\
	"dtb_reserve="						\
	"if test -n \"$fb_addr\"; then "	\
	"fdt addr " __stringify(CONFIG_KERNEL_DTB_ADDR)";"	\
	"fdt resize;"						\
	"fdt mk /reserved-memory display_reserved;"		\
	"fdt set /reserved-memory/display_reserved reg <$fb_addr 0x300000>;" \
	"fi;\0"

#define CONFIG_EXTRA_ENV_CMD_BOOT_ARGS				\
	"bootargs=console=ttySAC3,115200n8 "			\
	"root=/dev/mmcblk0p3 rw rootfstype=ext4 rootwait "	\
	"loglevel=4 quiet printk.time=1 consoleblank=0 "	\
	"systemd.log_level=info systemd.show_status=false\0"

#define CONFIG_EXTRA_ENV_CMD_BOOT_ARGS_RAMDISK			\
        "setenv bootargs console=ttySAC3,115200n8 " \
        "root=/dev/ram0 loglevel=4 quiet printk.time=1 consoleblank=0 " \
        "nx_drm.fb_buffers=3;"

#define CONFIG_EXTRA_ENV_KERNEL_LOAD				\
	"ext4load mmc 0:1 0x40080000 Image;"

#define CONFIG_EXTRA_ENV_DTB_LOAD	\
	"ext4load mmc 0:1 " __stringify(CONFIG_KERNEL_DTB_ADDR)	\
	" s5p6818-avn-ref-rev01.dtb;"				\
	"run dtb_reserve;"

#define CONFIG_EXTRA_ENV_RAMDISK_LOAD				\
	"ext4load mmc 0:1 0x48000000 uInitrd;"

#define CONFIG_EXTRA_ENV_CMD_RUN_KERNEL				\
	"booti 0x40080000 - "					\
	__stringify(CONFIG_KERNEL_DTB_ADDR)"\0"

#define CONFIG_EXTRA_ENV_CMD_RUN_KERNEL_FOR_INITRAMFS		\
	"booti 0x40080000 0x48000000 "				\
	__stringify(CONFIG_KERNEL_DTB_ADDR)"\0"

#define CONFIG_RECOVERY_BOOT_CMD	\
	"recoveryboot=not supported\0"

#define CONFIG_AUTORECOVERY_CMD		\
	"autorecovery_cmd=none\0"

#define CONFIG_EXTRA_OTA_AB_UPDATE \
        "misc_partition_num=" __stringify(CONTROL_PARTITION) "\0"       \
        "set_ab_select=" \
                SET_AB_SELECT \
                "\0" \
        "set_bootargs_ab1=setenv bootargs \"${bootargs} ${bootargs_ab1}\" \0" \
        "set_bootargs_ab2=setenv bootargs \"${bootargs} ${bootargs_ab2}\" \0" \
	"change_devicetree=run set_camera_input\0" \
	"set_camera_input=" \
	"fdt addr 41688000;fdt resize;"	\
	"if test ${cam_input} -eq 0; then " \
		"fdt set /soc/clipper2 status okay;" \
		"fdt set /soc/decimator2 status okay;" \
		"fdt set /soc/clipper9 status disabled;" \
		"fdt set /soc/decimator9 status disabled;" \
	"elif test ${cam_input} -eq 1; then " \
		"fdt set /soc/clipper2 status disabled;" \
		"fdt set /soc/decimator2 status disabled;" \
		"fdt set /soc/clipper9 status okay;" \
		"fdt set /soc/decimator9 status okay;" \
	"fi;\0" \
        "bootcmd_set_ab=run set_ab_select;" \
                       "run set_bootargs_ab1;" \
                       "run set_bootargs_ab2;" \
                       "\0"                    \
        "bootcmd=run bootcmd_set_ab;run android_boot_ab\0"

#define CONFIG_SYS_EXTRA_ENV_RELOC
#define CONFIG_EXTRA_ENV_SETTINGS				\
	"fdt_high=0xffffffffffffffff\0"				\
	CONFIG_EXTRA_ENV_CMD_BOOT_ARGS				\
	"boot_cmd_mmcboot="					\
		CONFIG_EXTRA_ENV_KERNEL_LOAD			\
		CONFIG_EXTRA_ENV_DTB_LOAD			\
		CONFIG_EXTRA_ENV_CMD_RUN_KERNEL			\
	CONFIG_RECOVERY_BOOT_CMD				\
	CONFIG_AUTORECOVERY_CMD					\
	"mmcboot=run boot_cmd_mmcboot\0"			\
	CONFIG_EXTRA_OTA_AB_UPDATE				\
	CONFIG_EXTRA_ENV_BOOT_LOGO				\
        "boot_cmd_ramfsboot="					\
		CONFIG_EXTRA_ENV_KERNEL_LOAD			\
		CONFIG_EXTRA_ENV_RAMDISK_LOAD			\
		CONFIG_EXTRA_ENV_DTB_LOAD			\
		CONFIG_EXTRA_ENV_CMD_RUN_KERNEL_FOR_INITRAMFS	\
        "ramfsboot=" \
	        CONFIG_EXTRA_ENV_CMD_BOOT_ARGS_RAMDISK		\
                "run boot_cmd_ramfsboot \0"

#endif /* __CONFIG_H__ */
