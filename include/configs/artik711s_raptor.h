/*
 * (C) Copyright 2018 Samsung Electronics
 * Chanho Park <chanho61.park@samsung.com>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#ifndef __ARTIK711S_RAPTOR_CONFIG_H__
#define __ARTIK711S_RAPTOR_CONFIG_H__

#include "artik710_raptor.h"

#define CONFIG_OF_BOARD_SETUP

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
	"load_fdt=\n"							\
	"    if test -z \"$fdtfile\"; then\n"				\
	"        loop=$board_rev\n"					\
	"        number=$board_rev:\n"					\
	"        success=0\n"						\
	"        until test $loop -eq 0 || test $success -ne 0; do\n"	\
	"            if test $loop -lt 10; then\n"			\
	"                number=0$loop\n"				\
	"            else\n"						\
	"                number=$loop\n"				\
	"            fi\n"						\
	"            ext4size mmc $rootdev:$bootpart s5p6818-artik711s-raptor-rev${number}.dtb && setexpr success 1\n" \
	"            setexpr loop $loop - 1\n"				\
	"        done\n"						\
	"        if test $success -eq 0; then\n"			\
	"            ext4load mmc $rootdev:$bootpart $fdtaddr s5p6818-artik711s-raptor-rev00.dtb || " \
			"ext4load mmc $rootdev:$bootpart $fdtaddr s5p6818-artik711s-raptor.dtb\n" \
	"        else\n"						\
	"            ext4load mmc $rootdev:$bootpart $fdtaddr s5p6818-artik711s-raptor-rev${number}.dtb\n" \
	"        fi\n"							\
	"    else\n"							\
	"        ext4load mmc $rootdev:$bootpart $fdtaddr $fdtfile\n"	\
	"    fi\n"							\
	"    setenv success\n"						\
	"    setenv number\n"						\
	"    setenv loop\0"						\
	"bootdelay=" __stringify(CONFIG_BOOTDELAY) "\0"			\
	"console=" CONFIG_DEFAULT_CONSOLE "\0"				\
	"consoleon=\n"							\
	"    setenv console " CONFIG_DEFAULT_CONSOLE "\n"		\
	"    saveenv\n"							\
	"    reset\0"							\
	"consoleoff=\n"							\
	"    setenv console console=ram\n"				\
	"    saveenv\n"							\
	"    reset\0"							\
	"rootdev=" __stringify(CONFIG_ROOT_DEV) "\0"			\
	"rootpart=" __stringify(CONFIG_ROOT_PART) "\0"			\
	"bootpart=" __stringify(CONFIG_BOOT_PART) "\0"			\
	"rescue=0\0"							\
	"root_rw=rw\0"							\
	"opts=\0"							\
	"loglevel=loglevel=4 splash\0"					\
	"rootfs_type=ext4\0"						\
	"sdrecovery=\n"							\
	"    run boot_cmd_sdboot\n"					\
	"    sd_recovery mmc 1:3 48000000 partmap_emmc.txt\0"		\
	"factory_load=factory_info load mmc 0 "				\
		__stringify(CONFIG_FACTORY_INFO_START) " "		\
		__stringify(CONFIG_FACTORY_INFO_SIZE) "\0"		\
	"factory_save=factory_info save mmc 0 "				\
		__stringify(CONFIG_FACTORY_INFO_START) " "		\
		__stringify(CONFIG_FACTORY_INFO_SIZE) "\0"		\
	"factory_set_ethaddr=\n"					\
	"    run factory_load\n"					\
	"    gen_eth_addr\n"						\
	"    factory_info write ethaddr $ethaddr"			\
	"    run factory_save\0"					\
	"load_args=\n"							\
	"    run factory_load\n"					\
	"    setenv bootargs ${console} "				\
		"root=/dev/mmcblk${rootdev}p${rootpart} ${root_rw} "	\
		"rootfstype=${rootfs_type} ${loglevel} ${opts} "	\
		"${recoverymode} "					\
		"${ota} bootfrom=${bootpart} rescue=${rescue}\0"	\
	"load_kernel=\n"						\
	"    ret=0\n"							\
	"    ext4load mmc ${rootdev}:${bootpart} $kerneladdr $kernel_file && setexpr ret 1\n" \
	"    if test $ret -eq 0; then\n"				\
	"        if test $bootpart -eq 2; then\n"			\
	"            setenv bootpart 3\n"				\
	"        else\n"						\
	"            setenv bootpart 2\n"				\
	"        fi\n"							\
	"        setenv rescue 1\n"					\
	"        ext4load mmc ${rootdev}:${bootpart} $kerneladdr $kernel_file\n" \
	"        run load_args\n"					\
	"    fi\0"							\
	"load_initrd=ext4load mmc ${rootdev}:${bootpart} $ramdiskaddr $ramdisk_file\0" \
	"boot_cmd_initrd=\n"						\
	"    run load_kernel\n"						\
	"    run load_fdt\n"						\
	"    run load_initrd\n"						\
	"    booti $kerneladdr $ramdiskaddr $fdtaddr\0"			\
	"boot_cmd_mmcboot=\n"						\
	"    run load_kernel\n"						\
	"    run load_fdt\n"						\
	"    booti $kerneladdr - $fdtaddr\0"				\
	"boot_cmd_sdboot=\n"						\
	"    setenv bootpart " __stringify(CONFIG_BOOT_PART_SD) "\n"	\
	"    setenv rootpart " __stringify(CONFIG_ROOT_PART_SD) "\0"	\
	"ramfsboot=\n"							\
	"    run load_args\n"						\
	"    run boot_cmd_initrd\0"					\
	"mmcboot=\n"							\
	"    run load_args\n"						\
	"    run boot_cmd_mmcboot\0"					\
	"recovery_cmd=\n"						\
	"    run sdrecovery\n"						\
	"    setenv recoverymode recovery\0"				\
	"recoveryboot=\n"						\
	"    run recovery_cmd\n"					\
	"    run ramfsboot\0"						\
	"hwtestboot=\n"							\
	"    setenv rootdev 1\n"					\
	"    setenv opts rootfstype=ext4 rootwait loglevel=4\n"		\
	"    setenv fdtfile s5p6818-artik711s-explorer.dtb\n"		\
	"    run mmcboot\0"						\
	"hwtest_recoveryboot=\n"					\
	"    run recovery_cmd\n"					\
	"    run hwtestboot\0"						\
	"bootcmd=run ramfsboot\0"

#endif /* __ARTIK711S_RAPTOR_CONFIG_H__ */
