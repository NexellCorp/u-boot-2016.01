// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2018 Linaro Ltd.
 * Sam Protsenko <semen.protsenko@linaro.org>
 */

#include <common.h>
#include <mapmem.h>
#include <android_image.h>

static int do_aboot_dump(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	char *endp;
	ulong load_addr;
	const struct andr_img_hdr *hdr;

	if (argc != 2)
		return CMD_RET_USAGE;

	load_addr = simple_strtoul(argv[1], &endp, 16);
	if (*endp != '\0') {
		printf("Error: Wrong image address\n");
		return CMD_RET_FAILURE;
	}
	hdr = map_sysmem(load_addr, sizeof(*hdr));

	if (android_image_check_header(hdr)) {
		printf("Error: boot image header is incorrect\n");
		return CMD_RET_FAILURE;
	}
	printf("kernel_size = %d \n", hdr->kernel_size);
	printf("kernel_addr = 0x%x \n", hdr->kernel_addr);
	printf("ramdisk_size = %d \n", hdr->ramdisk_size);
	printf("ramdisk_addr = 0x%x \n", hdr->ramdisk_addr);
	printf("second_size = %d \n", hdr->second_size);
	printf("second_addr = 0x%x \n", hdr->second_addr);
	printf("tags_addr = 0x%x \n", hdr->tags_addr);
	printf("page_size = %d \n", hdr->page_size);
	printf("name = %s \n", hdr->name);
	printf("cmdline = %s \n", hdr->cmdline);
	unmap_sysmem(hdr);


	return CMD_RET_SUCCESS;
}


#ifdef CONFIG_CMD_MMC
static int do_aboot_load_mmc(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	char command[256] = {0, };
	const struct andr_img_hdr *hdr;
	u32 bootimg_blk;
	u32 count;
	u32 kernel_addr, ramdisk_addr;
	u32 kernel_size, ramdisk_size;
	u32 kernel_offset, ramdisk_offset;

	bootimg_blk = simple_strtoul(argv[1], 0, 16);
	kernel_addr = simple_strtoul(argv[2], 0, 16);
	ramdisk_addr = simple_strtoul(argv[3], 0, 16);

	/* Step 1 : read bootimage_header(512byte) */
	count = 1;
	sprintf(command, "mmc read 0x%x 0x%x 0x%x", kernel_addr, bootimg_blk, count);
	run_command(command, 0);

	hdr = map_sysmem(kernel_addr, sizeof(*hdr));

	kernel_size = hdr->kernel_size;
	ramdisk_size = hdr->ramdisk_size;

	kernel_offset = bootimg_blk + 4;
	ramdisk_offset = kernel_offset + (kernel_size / 512) + 1;

	unmap_sysmem(hdr);

	/* Step 2 : load kernel to address  */
	count = (kernel_size / 512) + 1;
	sprintf(command, "mmc read 0x%x 0x%x 0x%x",
			kernel_addr, kernel_offset, count);
	run_command(command, 0);

	/* Step 3 : load rammdisk to address  */
	count = (ramdisk_size / 512) + 1;
	sprintf(command, "mmc read 0x%x 0x%x 0x%x",
			ramdisk_addr, ramdisk_offset, count);
	run_command(command, 0);

	sprintf(command, "%x", ramdisk_size);
	setenv("ramdisk_size", command);


	return CMD_RET_SUCCESS;
}
#endif

static cmd_tbl_t cmd_dtimg_sub[] = {
	U_BOOT_CMD_MKENT(dump, 2, 0, do_aboot_dump, "", ""),
#ifdef CONFIG_CMD_MMC
	U_BOOT_CMD_MKENT(load_mmc, 4, 0, do_aboot_load_mmc, "", ""),
#endif
};

static int do_aboot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *cp;

	cp = find_cmd_tbl(argv[1], cmd_dtimg_sub, ARRAY_SIZE(cmd_dtimg_sub));

	/* Strip off leading 'dtimg' command argument */
	argc--;
	argv++;

	if (!cp || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cp->repeatable)
		return CMD_RET_SUCCESS;

	return cp->cmd(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(
	aboot, CONFIG_SYS_MAXARGS, 0, do_aboot,
	"manipulate Android boot image",
	"dump <addr>\n"
	"    - parse specified image and print its structure info\n"
	"      <addr>: image address in RAM, in hex\n"
	"aboot load_mmc <boot.img offset> <kernel address> <ramdisk address>\n"
	"    - load from mmc from the image \n"
	"      <boot.img offset>: blk offset of boot.img on mmc, in hex \n"
	"      <kernel address>: kernel load RAM address \n"
	"      <ramdisk address>: ramdisk load RAM address "
);
