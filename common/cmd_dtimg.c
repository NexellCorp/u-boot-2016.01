// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2018 Linaro Ltd.
 * Sam Protsenko <semen.protsenko@linaro.org>
 */

#include <image-android-dt.h>
#include <dt_table.h>
#include <common.h>
#include <libfdt.h>
#include <mapmem.h>

enum cmd_dtimg_info {
	CMD_DTIMG_START = 0,
	CMD_DTIMG_SIZE,
};

static int do_dtimg_dump(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	char *endp;
	ulong hdr_addr;

	if (argc != 2)
		return CMD_RET_USAGE;

	hdr_addr = simple_strtoul(argv[1], &endp, 16);
	if (*endp != '\0') {
		printf("Error: Wrong image address\n");
		return CMD_RET_FAILURE;
	}

	if (!android_dt_check_header(hdr_addr)) {
		printf("Error: DT image header is incorrect\n");
		return CMD_RET_FAILURE;
	}

	android_dt_print_contents(hdr_addr);

	return CMD_RET_SUCCESS;
}

static int dtimg_get_fdt(int argc, char * const argv[], enum cmd_dtimg_info cmd)
{
	ulong hdr_addr;
	u32 index;
	char *endp;
	ulong fdt_addr;
	u32 fdt_size;
	char buf[65];

	if (argc != 4)
		return CMD_RET_USAGE;

	hdr_addr = simple_strtoul(argv[1], &endp, 16);
	if (*endp != '\0') {
		printf("Error: Wrong image address\n");
		return CMD_RET_FAILURE;
	}

	if (!android_dt_check_header(hdr_addr)) {
		printf("Error: DT image header is incorrect\n");
		return CMD_RET_FAILURE;
	}

	index = simple_strtoul(argv[2], &endp, 0);
	if (*endp != '\0') {
		printf("Error: Wrong index\n");
		return CMD_RET_FAILURE;
	}

	if (!android_dt_get_fdt_by_index(hdr_addr, index, &fdt_addr, &fdt_size))
		return CMD_RET_FAILURE;

	switch (cmd) {
	case CMD_DTIMG_START:
		snprintf(buf, sizeof(buf), "%lx", fdt_addr);
		break;
	case CMD_DTIMG_SIZE:
		snprintf(buf, sizeof(buf), "%x", fdt_size);
		break;
	default:
		printf("Error: Unknown cmd_dtimg_info value: %d\n", cmd);
		return CMD_RET_FAILURE;
	}

	setenv(argv[3], buf);

	return CMD_RET_SUCCESS;
}

static int do_dtimg_start(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	return dtimg_get_fdt(argc, argv, CMD_DTIMG_START);
}

static int do_dtimg_size(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	return dtimg_get_fdt(argc, argv, CMD_DTIMG_SIZE);
}

#ifdef CONFIG_CMD_MMC
static int do_dtimg_load_mmc(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	char command[256] = {0, };
	const struct dt_table_header *hdr;
	const struct dt_table_entry *e;
	u32 dt_part_blk;
	u32 load_addr;
	u32 dt_id;
	u32 count;
	u32 entry_count;
	u32 magic;
	u32 dt_offset=0, dt_size=0, dt_table_id=0;
	u32 i;


	if (argc != 4)
		return CMD_RET_USAGE;

	dt_part_blk = simple_strtoul(argv[1], 0, 16);
	load_addr = simple_strtoul(argv[2], 0, 16);
	dt_id = simple_strtoul(argv[3], 0, 0);

	/* Step 1 : read dt_table_header(512byte) */
	count = 1;
	sprintf(command, "mmc read 0x%x 0x%x 0x%x", load_addr, dt_part_blk, count);
	run_command(command, 0);

	hdr = map_sysmem(load_addr, sizeof(*hdr));
	magic = fdt32_to_cpu(hdr->magic);
	entry_count = fdt32_to_cpu(hdr->dt_entry_count);
	unmap_sysmem(hdr);

	/* check dt id and dt_entry_count */
	if(magic != DT_TABLE_MAGIC) {
		printf("DT table MAGIC 0x%x mitmatch : %x \n", DT_TABLE_MAGIC, magic);
		return CMD_RET_FAILURE;
	}

	/* Step 2 : find id from entry and read offset and size */
	count = 1;
	for(i = 0; i < entry_count; i++) {
		sprintf(command, "mmc read 0x%x 0x%x 0x%x",
				load_addr, dt_part_blk+i+1, count);
		run_command(command, 0);

		e = map_sysmem(load_addr, sizeof(*e));
		dt_table_id = fdt32_to_cpu(e->id);
		if(dt_table_id == dt_id) {
			printf("find dt id %d in dtb.img \n", dt_id);
			dt_offset = fdt32_to_cpu(e->dt_offset);
			dt_size = fdt32_to_cpu(e->dt_size);
			unmap_sysmem(e);
			break;
		}
		unmap_sysmem(e);
	}

	if(dt_offset == 0 || dt_size==0) {
		printf("Can't find dt id %d in dtb.img \n", dt_id);
		return CMD_RET_FAILURE;
	}

	/* Step 3 : load dtb to address  */
	count = dt_size / 512;
	sprintf(command, "mmc read 0x%x 0x%x 0x%x",
			load_addr, dt_part_blk+(dt_offset/512), count);
	run_command(command, 0);

	return CMD_RET_SUCCESS;
}
#endif

static cmd_tbl_t cmd_dtimg_sub[] = {
	U_BOOT_CMD_MKENT(dump, 2, 0, do_dtimg_dump, "", ""),
	U_BOOT_CMD_MKENT(start, 4, 0, do_dtimg_start, "", ""),
	U_BOOT_CMD_MKENT(size, 4, 0, do_dtimg_size, "", ""),
#ifdef CONFIG_CMD_MMC
	U_BOOT_CMD_MKENT(load_mmc, 4, 0, do_dtimg_load_mmc, "", ""),
#endif
};

static int do_dtimg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
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
	dtimg, CONFIG_SYS_MAXARGS, 0, do_dtimg,
	"manipulate dtb/dtbo Android image",
	"dump <addr>\n"
	"    - parse specified image and print its structure info\n"
	"      <addr>: image address in RAM, in hex\n"
	"dtimg start <addr> <index> <varname>\n"
	"    - get address (hex) of FDT in the image, by index\n"
	"      <addr>: image address in RAM, in hex\n"
	"      <index>: index of desired FDT in the image\n"
	"      <varname>: name of variable where to store address of FDT\n"
	"dtimg size <addr> <index> <varname>\n"
	"    - get size (hex, bytes) of FDT in the image, by index\n"
	"      <addr>: image address in RAM, in hex\n"
	"      <index>: index of desired FDT in the image\n"
	"      <varname>: name of variable where to store size of FDT\n"
	"dtimg load_mmc  <dt partition offset> <address> <dt id>\n"
	"    - load image from mmc \n"
	"      <dt parttion blk>: blk offset of dtb partition on mmc, in hex \n"
	"      <address>: destination of RAM address \n"
	"      <dt id> dtb id"
);
