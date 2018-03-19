/*
 * (C) Copyright 2018 Samsung Electronics
 * Chanho Park <chanho61.park@samsung.com>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#ifndef __ARTIK711S_RAPTOR_CONFIG_H__
#define __ARTIK711S_RAPTOR_CONFIG_H__

#include "artik710_raptor.h"

#undef CONFIG_DFU_ALT
#define CONFIG_DFU_ALT \
	"bl1-emmcboot.img raw 0x1 0x80;" \
	"bl1-sdboot.img raw 0x1 0x80;" \
	"fip-loader-emmc.img raw 0x81 0x280;" \
	"fip-loader-sd.img raw 0x81 0x280;" \
	"fip-secure.img raw 0x301 0x600;" \
	"fip-nonsecure.img raw 0xf01 0x800;" \
	"/uImage ext4 $rootdev $bootpart;" \
	"/Image ext4 $rootdev $bootpart;" \
	"/uInitrd ext4 $rootdev $bootpart;" \
	"/ramdisk.gz ext4 $rootdev $bootpart;" \
	"/s5p6818-artik711s-raptor-rev03.dtb ext4 $rootdev $bootpart;" \
	"/s5p6818-artik711s-raptor-rev02.dtb ext4 $rootdev $bootpart;" \
	"/s5p6818-artik711s-raptor-rev01.dtb ext4 $rootdev $bootpart;" \
	"boot part $rootdev $bootpart;" \
	"modules part $rootdev $modulespart;" \
	"rootfs part $rootdev $rootpart;" \
	"params.bin raw 0x1701 0x20;" \
	"/Image.itb ext4 $rootdev $bootpart\0"

#undef CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS					\
	"fdt_high=0xffffffffffffffff\0"					\
	"kerneladdr=0x40080000\0"					\
	"kernel_file=Image\0"						\
	"ramdiskaddr=0x49000000\0"					\
	"ramdisk_file=uInitrd\0"					\
	"fdtaddr=0x4a000000\0"						\
	"fdtfile=\0"							\
	"load_fdt="							\
		"if test -z \"$fdtfile\"; then "			\
		"loop=$board_rev; "					\
		"number=$board_rev: "					\
		"success=0; "						\
		"until test $loop -eq 0 || test $success -ne 0; do "	\
			"if test $loop -lt 10; then "			\
				"number=0$loop; "			\
			"else number=$loop; "				\
			"fi; "						\
			"ext4size mmc $rootdev:$bootpart s5p6818-artik711s-raptor-rev${number}.dtb && setexpr success 1; " \
			"setexpr loop $loop - 1; "			\
		"done; "						\
		"if test $success -eq 0; then "				\
			"ext4load mmc $rootdev:$bootpart $fdtaddr s5p6818-artik711s-raptor-rev00.dtb || "	\
			"ext4load mmc $rootdev:$bootpart $fdtaddr s5p6818-artik711s-raptor.dtb; "	\
		"else "							\
			"ext4load mmc $rootdev:$bootpart $fdtaddr s5p6818-artik711s-raptor-rev${number}.dtb; "	\
		"fi; "							\
		"else ext4load mmc $rootdev:$bootpart $fdtaddr $fdtfile; " \
		"fi; setenv success; setenv number; setenv loop;\0"	\
	"bootdelay=" __stringify(CONFIG_BOOTDELAY) "\0"			\
	"console=" CONFIG_DEFAULT_CONSOLE				\
	"consoleon=setenv console=" CONFIG_DEFAULT_CONSOLE		\
		"; saveenv; reset\0"					\
	"consoleoff=setenv console=ram; saveenv; reset\0"		\
	"rootdev=" __stringify(CONFIG_ROOT_DEV) "\0"			\
	"rootpart=" __stringify(CONFIG_ROOT_PART) "\0"			\
	"bootpart=" __stringify(CONFIG_BOOT_PART) "\0"			\
	"rescue=0\0"							\
	"root_rw=rw\0"							\
	"opts=\0"							\
	"loglevel=loglevel=4 splash\0"					\
	"rootfs_type=ext4\0"						\
	"sdrecovery=run boot_cmd_sdboot;"				\
		"sd_recovery mmc 1:3 48000000 partmap_emmc.txt\0"	\
	"factory_load=factory_info load mmc 0 "				\
		__stringify(CONFIG_FACTORY_INFO_START) " "		\
		__stringify(CONFIG_FACTORY_INFO_SIZE) "\0"		\
	"factory_save=factory_info save mmc 0 "				\
		__stringify(CONFIG_FACTORY_INFO_START) " "		\
		__stringify(CONFIG_FACTORY_INFO_SIZE) "\0"		\
	"factory_set_ethaddr=run factory_load; gen_eth_addr ;"		\
		"factory_info write ethaddr $ethaddr;"			\
		"run factory_save\0"					\
	"load_args=run factory_load; setenv bootargs ${console} "	\
		"root=/dev/mmcblk${rootdev}p${rootpart} ${root_rw} "	\
		"rootfstype=${rootfs_type} ${loglevel} ${opts} "	\
		"${recoverymode} "					\
		"${ota} bootfrom=${bootpart} rescue=${rescue};\0"	\
	"load_kernel="							\
		"ret=0; "						\
		"ext4load mmc ${rootdev}:${bootpart} $kerneladdr $kernel_file && setexpr ret 1; " \
		"if test $ret -eq 0; then "				\
			"if test $bootpart -eq 2; then "		\
				"setenv bootpart 3; "			\
			"else setenv bootpart 2; "			\
			"fi; "						\
			"setenv rescue 1; "				\
			"ext4load mmc ${rootdev}:${bootpart} $kerneladdr $kernel_file; " \
			"run load_args; "				\
		"fi;\0"							\
	"load_initrd=ext4load mmc ${rootdev}:${bootpart} $ramdiskaddr $ramdisk_file\0" \
	"boot_cmd_initrd="						\
		"run load_kernel; run load_fdt; run load_initrd;"	\
		"booti $kerneladdr $ramdiskaddr $fdtaddr\0"		\
	"boot_cmd_mmcboot="						\
		"run load_kernel; run load_fdt;"			\
		"booti $kerneladdr - $fdtaddr\0"			\
	"boot_cmd_sdboot="						\
		"setenv bootpart " __stringify(CONFIG_BOOT_PART_SD)"; "	\
		"setenv rootpart " __stringify(CONFIG_ROOT_PART_SD)";\0"	\
	"ramfsboot=run load_args; run boot_cmd_initrd\0"		\
	"mmcboot=run load_args; run boot_cmd_mmcboot\0"			\
	"recovery_cmd=run sdrecovery; setenv recoverymode recovery\0"	\
	"recoveryboot=run recovery_cmd; run ramfsboot\0"		\
	"hwtestboot=setenv rootdev 1;"					\
		"setenv opts rootfstype=ext4 rootwait loglevel=4;"	\
		"setenv fdtfile s5p6818-artik711s-explorer.dtb; "	\
		"run mmcboot\0"						\
	"hwtest_recoveryboot=run recovery_cmd; run hwtestboot\0"	\
	"bootargs=none\0"						\
	"bootcmd=run ramfsboot\0"

#endif /* __ARTIK711S_RAPTOR_CONFIG_H__ */
