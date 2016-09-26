/*
 * (C) Copyright 2016 Nexell
 * Hyunseok, Jung <hsjung@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <linux/sizes.h>
#include <asm/arch/nexell.h>
/*-----------------------------------------------------------------------
 *  u-boot-2016.01
 */
#define CONFIG_SYS_LDSCRIPT "arch/arm/cpu/armv7/s5p4418/u-boot.lds"

#define	CONFIG_MACH_S5P4418
#define CONFIG_SYS_THUMB_BUILD
#define	CONFIG_SKIP_LOWLEVEL_INIT

/*-----------------------------------------------------------------------
 * PLL
 */
#define CONFIG_SYS_PLLFIN			24000000UL

/*-----------------------------------------------------------------------
 *  System memory Configuration
 */

#define	CONFIG_SYS_TEXT_BASE			0x43C00000
/* init and run stack pointer */
#define	CONFIG_SYS_INIT_SP_ADDR			CONFIG_SYS_TEXT_BASE

/* malloc() pool */
#define	CONFIG_MEM_MALLOC_START			0x44000000
/* more than 2M for ubifs: MAX 16M */
#define CONFIG_MEM_MALLOC_LENGTH		(32*1024*1024)

/* Download OFFSET */
#define CONFIG_MEM_LOAD_ADDR			0x48000000

#define CONFIG_SYS_BOOTM_LEN    (64 << 20)      /* Increase max gunzip size */

/*-----------------------------------------------------------------------
 *  High Level System Configuration
 */

/* Not used: not need IRQ/FIQ stuff	*/
#undef  CONFIG_USE_IRQ
/* decrementer freq: 1ms ticks */
#define CONFIG_SYS_HZ				1000

/* board_init_f */
#define	CONFIG_SYS_SDRAM_BASE			0x40000000
#define	CONFIG_SYS_SDRAM_SIZE			0x20000000

/* dram 1 bank num */
#define CONFIG_NR_DRAM_BANKS			1

/* relocate_code and  board_init_r */
#define	CONFIG_SYS_MALLOC_END			(CONFIG_MEM_MALLOC_START + \
						 CONFIG_MEM_MALLOC_LENGTH)
/* board_init_f, more than 2M for ubifs */
#define CONFIG_SYS_MALLOC_LEN \
	(CONFIG_MEM_MALLOC_LENGTH - 0x8000)

/* kernel load address */
#define CONFIG_SYS_LOAD_ADDR			CONFIG_MEM_LOAD_ADDR

/*-----------------------------------------------------------------------
 *  System initialize options (board_init_f)
 *  if disabel mmu, set CONFIG_SYS_DCACHE_OFF
 */

/* board_init_f->init_sequence, call arch_cpu_init */
#define CONFIG_ARCH_CPU_INIT

/* board_init_r, call board_early_init_f */
#define	CONFIG_BOARD_LATE_INIT

/* board_init_f->init_sequence, call print_cpuinfo */
#define	CONFIG_DISPLAY_CPUINFO

/*-----------------------------------------------------------------------
 *	U-Boot default cmd
 */
#ifdef CONFIG_CMD_MEMTEST
#define CONFIG_SYS_MEMTEST_START		CONFIG_SYS_MALLOC_END
#define CONFIG_SYS_MEMTEST_END			((ulong)CONFIG_SYS_SDRAM_BASE \
						 + (ulong)CONFIG_SYS_SDRAM_SIZE)
#endif

/*-----------------------------------------------------------------------
 * Miscellaneous configurable options
 */
#ifdef CONFIG_SYS_PROMPT
#undef CONFIG_SYS_PROMPT
/* Monitor Command Prompt   */
#define CONFIG_SYS_PROMPT			"s5p4418_navi_ref# "
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
 * Default Boot Environments
 */
#define CONFIG_BOOTDELAY			1
#define CONFIG_ZERO_BOOTDELAY_CHECK
#define CONFIG_SILENT_CONSOLE

#define CONFIG_BOOTCOMMAND	"ext4load mmc 0:1 0x40008000 Image;"	\
				"ext4load mmc 0:1 49000000 linux.dtb;"	\
				"bootl 0x40008000 - 49000000"

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
#define	CONFIG_CMD_IMI				/* image info	*/
#define CONFIG_CMDLINE_EDITING			/* add command line history */
#define	CONFIG_CMDLINE_TAG			/* use bootargs commandline */
#define	CONFIG_CMD_BOOTL			/* boot linux image */

#undef  CONFIG_BOOTM_NETBSD
#undef  CONFIG_BOOTM_PLAN9
#undef  CONFIG_BOOTM_RTEMS
#undef  CONFIG_BOOTM_VXWORKS

/*-----------------------------------------------------------------------
 * serial console configuration
 */
#define CONFIG_PL011_SERIAL
#define CONFIG_CONS_INDEX			3
#define CONFIG_PL011_CLOCK			50000000
#define CONFIG_PL01x_PORTS			{(void *)PHY_BASEADDR_UART0, \
						(void *)PHY_BASEADDR_UART1, \
						(void *)PHY_BASEADDR_UART2, \
						(void *)PHY_BASEADDR_UART3}

#define CONFIG_BAUDRATE				115200
#define CONFIG_SYS_BAUDRATE_TABLE \
		{ 9600, 19200, 38400, 57600, 115200 }
#define CONFIG_PL011_SERIAL_FLUSH_ON_INIT

/*-----------------------------------------------------------------------
 * NOR FLASH
 */
#define	CONFIG_SYS_NO_FLASH

/*-----------------------------------------------------------------------
 * Timer
 */
#define CONFIG_TIMER_SYS_TICK_CH		0

/*-----------------------------------------------------------------------
 * SD/MMC
 */
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_DWMMC
#define CONFIG_NEXELL_DWMMC
#define CONFIG_BOUNCE_BUFFER

#ifdef CONFIG_MMC
#define CONFIG_2NDBOOT_OFFSET		512
#define CONFIG_2NDBOOT_SIZE		(64*1024)
#define CONFIG_FIP_OFFSET		(CONFIG_2NDBOOT_OFFSET +\
					 CONFIG_2NDBOOT_SIZE)
#define CONFIG_FIP_SIZE			(3*1024*1024)

#define CONFIG_ENV_IS_IN_MMC

#ifdef CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0
#define	CONFIG_ENV_OFFSET		(CONFIG_FIP_OFFSET +\
					 CONFIG_FIP_SIZE)
#define CONFIG_ENV_SIZE			(512)	/* env size */
#endif

#ifndef CONFIG_FIT
/* refer to struct nx_dwmci_dat at nexell_dw_mmc.c */
#define	CONFIG_NEXELL_DWMMC_PLATFORM_DATA {	\
	[0] = { \
		.dev = 0, \
		.frequency = 50000000, \
		.buswidth = 4,	\
		.d_delay = 0x0, \
		.d_shift = 0x03, \
		.s_delay = 0x00, \
		.s_shift = 0x02,	\
	}, \
}
#endif
#endif /* CONFIG_MMC */

#if defined(CONFIG_MMC)
#define CONFIG_CMD_MMC
#define CONFIG_DOS_PARTITION
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_EXT4_WRITE
#define CONFIG_FS_EXT4
#define CONFIG_EXT4_WRITE
#define CONFIG_CMD_FDISK
#define	CONFIG_CMD_EXT4_IMG_WRITE
#endif

/*-----------------------------------------------------------------------
 * Fastboot and USB OTG
 */
#define CONFIG_USB_FUNCTION_FASTBOOT
#define CONFIG_CMD_FASTBOOT

#define CONFIG_FASTBOOT_BUF_SIZE        (CONFIG_SYS_SDRAM_SIZE - SZ_1M)
#define CONFIG_FASTBOOT_BUF_ADDR        CONFIG_SYS_SDRAM_BASE

#define CONFIG_USB_GADGET
#define CONFIG_USB_GADGET_DUALSPEED
#define CONFIG_USB_GADGET_VBUS_DRAW     0
#define CONFIG_USB_GADGET_DWC2_OTG
#define CONFIG_USB_GADGET_NX_UDC_OTG_PHY

#define CONFIG_SYS_CACHELINE_SIZE       64

#define CONFIG_USB_GADGET_DOWNLOAD
#define CONFIG_G_DNL_VENDOR_NUM         0x18d1  /* google */
#define CONFIG_G_DNL_PRODUCT_NUM        0x0002  /* nexus one */
#define CONFIG_G_DNL_MANUFACTURER       "Nexell Corporation"

/*-----------------------------------------------------------------------
 * Nexell USB Downloader
 */
#define CONFIG_NX_USBDOWN

/*-----------------------------------------------------------------------
 * Default environment organization
 */
#if !defined(CONFIG_ENV_IS_IN_MMC) && !defined(CONFIG_ENV_IS_IN_NAND) && \
	!defined(CONFIG_ENV_IS_IN_FLASH) && !defined(CONFIG_ENV_IS_IN_EEPROM)
	#define CONFIG_ENV_IS_NOWHERE
	#define	CONFIG_ENV_OFFSET		1024
	#define CONFIG_ENV_SIZE			(4*1024)	/* env size */
	#undef	CONFIG_CMD_IMLS
#endif

/*-----------------------------------------------------------------------
 * SPL
 */
#ifdef CONFIG_SPL
/* SPL FRAMEWORK */
#if 1
#define	CONFIG_SPL_CLI_FRAMEWORK	/* set prompt mode */
#endif

/* Support Devices */
#define	CONFIG_SPL_SERIAL_SUPPORT
#define	CONFIG_SPL_MMC_SUPPORT
#define CONFIG_NX_USBDOWN

/*
 * Support commands
 */
#define CONFIG_CMD_EXT4

#include <configs/s5p4418_spl.h>

#endif /* CONFIG_SPL */
#endif /* __CONFIG_H__ */
