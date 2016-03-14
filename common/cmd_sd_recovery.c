/*
 * (C) Copyright 2016 Nexell
 * Youngbok, Park <ybpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <asm/byteorder.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <mmc.h>
#include <fat.h>
#include <fs.h>
#include <part.h>

#define	UPDATE_SDCARD_MMC_MAX		3
#define	UPDATE_SDCARD_EEPROM_MAX	1
#define	UPDATE_SDCARD_NAND_MAX		1
#define	UPDATE_SDCARD_MEM_MAX		1

#define	DEV_PART_MAX	(10)	/* each device max partition max num */

/* device types */
#define	UPDATE_SDCARD_DEV_EEPROM	(1<<0)	/*  name "eeprom" */
#define	UPDATE_SDCARD_DEV_NAND		(1<<1)	/*  name "nand" */
#define	UPDATE_SDCARD_DEV_MMC		(1<<2)	/*  name "mmc" */
#define	UPDATE_SDCARD_DEV_MEM		(1<<3)	/*  name "mem" */

/* filesystem types */
#define	UPDATE_SDCARD_FS_2NDBOOT	(1<<0)	/*  name "boot" <- bootable */
#define	UPDATE_SDCARD_FS_BOOT		(1<<1)	/*  name "boot" <- bootable */
#define	UPDATE_SDCARD_FS_RAW		(1<<2)	/*  name "raw" */
#define	UPDATE_SDCARD_FS_FAT		(1<<4)	/*  name "fat" */
#define	UPDATE_SDCARD_FS_EXT4		(1<<5)	/*  name "ext4" */
#define	UPDATE_SDCARD_FS_UBI		(1<<6)	/*  name "ubi" */
#define	UPDATE_SDCARD_FS_UBIFS		(1<<7)	/*  name "ubifs" */
#define	UPDATE_SDCARD_FS_RAW_PART	(1<<8)	/*  name "emmc" */

#define	UPDATE_SDCARD_FS_MASK		(UPDATE_SDCARD_FS_RAW |\
					 UPDATE_SDCARD_FS_FAT |\
					 UPDATE_SDCARD_FS_EXT4 |\
					 UPDATE_SDCARD_FS_UBI |\
					 UPDATE_SDCARD_FS_UBIFS |\
					 UPDATE_SDCARD_FS_RAW_PART)

#define	TCLK_TICK_HZ				(1000000)

struct update_sdcard_fs_type {
	char *name;
	unsigned int fs_type;
};

/* support fs type */
static struct update_sdcard_fs_type f_part_fs[] = {
	{ "2nd"		, UPDATE_SDCARD_FS_2NDBOOT	},
	{ "boot"	, UPDATE_SDCARD_FS_BOOT	},
	{ "raw"		, UPDATE_SDCARD_FS_RAW		},
	{ "fat"		, UPDATE_SDCARD_FS_FAT		},
	{ "ext4"	, UPDATE_SDCARD_FS_EXT4		},
	{ "emmc"	, UPDATE_SDCARD_FS_RAW_PART	},
	{ "ubi"		, UPDATE_SDCARD_FS_UBI		},
	{ "ubifs"	, UPDATE_SDCARD_FS_UBIFS		},
};

struct update_sdcard_part {
	char device[32];
	int dev_no;
	char partition_name[32];
	unsigned int fs_type;
	uint64_t start;
	uint64_t length;
	char file_name[32];
	int part_num;
	struct list_head link;
};

static struct update_sdcard_part f_sdcard_part[DEV_PART_MAX];


static int sdcard_mmc_check_part_table(block_dev_desc_t *desc,
					      struct update_sdcard_part *fpart)
{
	uint64_t parts[DEV_PART_MAX][2] = { {0, 0}, };
	int i = 0, num = 0;
	int ret = 1;

	if (0 > get_part_table(desc, parts, &num))
		return -1;

	for (i = 0; num > i; i++) {
		if (parts[i][0] == fpart->start &&
		    parts[i][1] == fpart->length)
			return 0;
		/* when last partition set value is zero,
		   set avaliable length */
		if ((num-1) == i &&
		    parts[i][0] == fpart->start &&
		    fpart->length == 0) {
			fpart->length = parts[i][1];
			ret = 0;
			break;
		}
	}
	return ret;
}

static inline void update_sdcard_parse_comment(const char *str,
					       const char **ret)
{
	const char *p = str, *r;

	do {
		r = strchr(p, '#');
		if (!r)
			break;
		r++;
		p = strchr(r, '\n');
		if (!p) {
			printf("---- not end comments '#' ----\n");
			break;
		}
		p++;
	} while (1);

	/* for next */
	*ret = p;
}

static inline int update_sdcard_parse_string(const char *s,
					     const char *e, char *b, int len)
{
	int l, a = 0;

	do {
		while (0x20 == *s || 0x09 == *s || 0x0a == *s)
			s++;
	} while (0);

	if (0x20 == *(e-1) || 0x09 == *(e-1))
		do {
			e--;
			while (0x20 == *e || 0x09 == *e)
				e--;
			a = 1;
		} while (0);

	l = (e - s + a);
	if (l > len) {
		printf("-- Not enough buffer %d for string len %d [%s] --\n",
		       len, l, s);
		return -1;
	}

	strncpy(b, s, l);
	b[l] = 0;

	return l;
}

static inline void update_sdcard_sort_string(char *p, int len)
{
	int i, j;
	for (i = 0, j = 0; len > i; i++) {
		if (0x20 != p[i] && 0x09 != p[i] && 0x0A != p[i])
			p[j++] = p[i];
	}
	p[j] = 0;
}


static int update_sdcard_parse_part_head(const char *parts, const char **ret)
{
	const char *p = parts;
	int len = strlen("flash=");

	debug("\n");
	p = strstr(p, "flash=");
	if (!p)
		return -1;

	*ret = p + len;
	return 0;
}


static const char *update_sdcard_get_string(const char *ptable_str,
					     int search_c,
					     char *buf,
					     int buf_size)
{
	const char *id, *c;

	id = ptable_str;
	c = strchr(id, search_c);

	memset(buf, 0x0, buf_size);
	update_sdcard_parse_string(id, c, buf, buf_size);

	return c+1;
}


static int update_sdcard_part_lists_make(const char *ptable_str,
					 int ptable_str_len)
{
	struct update_sdcard_part *fp = f_sdcard_part;
	const char *p;
	char str[32];
	int i = 0, j = 0;
	int err = -1;
	int part_num = 0;

	p = ptable_str;

	update_sdcard_parse_comment(p, &p);
	update_sdcard_sort_string((char *)p, ptable_str_len);

	for (i = 0; i < DEV_PART_MAX; i++, fp++) {
		struct update_sdcard_fs_type *fs = f_part_fs;

		if (update_sdcard_parse_part_head(p, &p))
			break;

		p = update_sdcard_get_string(p, ',', str, sizeof(str));
		strcpy(fp->device, str);

		p = update_sdcard_get_string(p, ':', str, sizeof(str));
		fp->dev_no = simple_strtoul(str, NULL, 10);

		p = update_sdcard_get_string(p, ':', str, sizeof(str));
		strcpy(fp->partition_name, str);


		p = update_sdcard_get_string(p, ':', str, sizeof(str));

		for (j = 0; ARRAY_SIZE(f_part_fs) > j; j++, fs++) {
			if (strcmp(fs->name, str) == 0) {
				fp->fs_type = fs->fs_type;

				if (fp->fs_type & UPDATE_SDCARD_FS_MASK) {
					part_num++;
					fp->part_num = part_num;
				}
				break;
			}
		}

		p = update_sdcard_get_string(p, ',', str, sizeof(str));
		fp->start = simple_strtoul(str, NULL, 16);

		p = update_sdcard_get_string(p, ':', str, sizeof(str));
		fp->length = simple_strtoul(str, NULL, 16);

		p = update_sdcard_get_string(p, ';', str, sizeof(str));
		strcpy(fp->file_name, str);

		err = 0;
	}

	return err;
}

static void update_sdcard_part_lists_print(void)
{
	struct update_sdcard_part *fp = f_sdcard_part;
	int i;

	printf("\nPartitions:\n");

	for (i = 0; i < DEV_PART_MAX; i++, fp++) {
		if (!strcmp(fp->device, ""))
			break;

		printf("  %s.%d : %s : %s : 0x%llx, 0x%llx : %s , %d\n",
		       fp->device, fp->dev_no,
		       fp->partition_name,
		       UPDATE_SDCARD_FS_MASK&fp->fs_type ? "fs" : "img",
		       fp->start, fp->length,
		       fp->file_name, fp->part_num);
	}
}

int update_sd_do_load(cmd_tbl_t *cmdtp, int flag, int argc,
		      char * const argv[], int fstype, int cmdline_base)
{
	unsigned long addr;
	const char *addr_str;
	const char *filename;
	unsigned long bytes;
	unsigned long pos;
	loff_t len_read;
	char buf[12];
	unsigned long time;
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;
	if (argc > 7)
		return CMD_RET_USAGE;

	if (fs_set_blk_dev(argv[1], (argc >= 3) ? argv[2] : NULL, fstype))
		return -1;

	if (argc >= 4) {
		addr = simple_strtoul(argv[3], NULL, cmdline_base);
	} else {
		addr_str = getenv("loadaddr");
		if (addr_str != NULL)
			addr = simple_strtoul(addr_str, NULL, 16);
		else
			addr = CONFIG_SYS_LOAD_ADDR;
	}
	if (argc >= 5) {
		filename = argv[4];
	} else {
		filename = getenv("bootfile");
		if (!filename) {
			puts("** No boot file defined **\n");
			return -1;
		}
	}
	if (argc >= 6)
		bytes = simple_strtoul(argv[5], NULL, cmdline_base);
	else
		bytes = 0;
	if (argc >= 7)
		pos = simple_strtoul(argv[6], NULL, cmdline_base);
	else
		pos = 0;

	time = get_timer(0);

	ret = fs_read(filename, addr, pos, bytes, &len_read);
	time = get_timer(time);
	if (len_read <= 0 || ret < 0)
		return -1;

	printf("%lld bytes read in %lu ms", len_read, time);

	if (time > 0) {
		puts(" (");
		print_size(len_read / time * 1000, "/s");
		puts(")");
	}
	puts("\n");

	sprintf(buf, "0x%llx", len_read);

	return len_read;
}

static int do_update_sdcard(cmd_tbl_t *cmdtp, int flag, int argc,
			    char * const argv[])
{
	char *p;
	unsigned long addr;
	unsigned long time;
	int i = 0;
	int res = 0;
	int len_read = 0;
	int err = 0;
	loff_t len;

	if (argc != 5) {
		printf("fail parse Args! ");
		goto ret_error;
	}

	memset(f_sdcard_part, 0x0, sizeof(f_sdcard_part));

	len_read = update_sd_do_load(cmdtp, flag, argc, argv, FS_TYPE_FAT, 16);
	printf(" len : %d ", len_read);

	if (len_read <= 0) {
		printf(" read Partmap file error!\n");
		return -1;
	}

	/* partition map parse */
	addr = simple_strtoul(argv[3], NULL, 16);
	p = (char *)addr;
	p[len_read+1] = '\0';
	update_sdcard_sort_string((char *)p, len_read);

	err = update_sdcard_part_lists_make(p, strlen(p));
	if (err < 0) {
		printf(" partition map list parse fail\n");
		return -1;
	}
	struct update_sdcard_part *fp = f_sdcard_part;

	update_sdcard_part_lists_print();
	printf("\n");

	for (i = 0; i < DEV_PART_MAX; i++, fp++) {
		if (!strcmp(fp->device, ""))
			break;

		if (!strcmp(fp->file_name, "dummy"))
			continue;

		if (fs_set_blk_dev(argv[1], (argc >= 3) ? argv[2] : NULL,
				   FS_TYPE_FAT)) {
			printf("Block device set err!\n");
			goto ret_error;
		}

		time = get_timer(0);
		len_read = fs_read(fp->file_name, addr, 0, 0, &len);
		time = get_timer(time);

		printf("%lld bytes read in %lu ms", len, time);
		if (time > 0) {
			puts(" (");
			print_size(len / time * 1000, "/s");
			puts(")");
		}
		puts("\n");

		debug("%s.%d : %s : %s : 0x%llx, 0x%llx : %s\n", fp->device,
		      fp->dev_no, fp->partition_name,
		      UPDATE_SDCARD_FS_MASK&fp->fs_type ? "fs" : "img",
		      fp->start, fp->length, fp->file_name);

		if (len <= 0)
			continue;

		block_dev_desc_t *desc;
		char cmd[128];
		int i = 0, l = 0, p = 0;
		char *device = fp->device;
		char *partition_name = fp->partition_name;
		char *file_name = fp->file_name;
		uint64_t start = fp->start;
		uint64_t length = fp->length;
		int dev = fp->dev_no;
		unsigned int fs_type = fp->fs_type;
		int part_num = fp->part_num;

		length = len;

		memset(cmd, 0x0, sizeof(cmd));

		if (!strcmp(device, "mmc")) {
			sprintf(cmd, "mmc dev %d", dev);
			printf("** mmc.%d partition %s (%s)**\n",
			       dev, partition_name,
			       fs_type&UPDATE_SDCARD_FS_EXT4 ? "FS" : "Image");

			/* set mmc devicee */
			if (0 > get_device("mmc", simple_itoa(dev), &desc)) {
				if (0 > run_command(cmd, 0)) {
					printf("MMC get device err\n");
					goto ret_error;
				}

				if (0 > run_command("mmc rescan", 0)) {
					printf("MMC get device err\n");
					goto ret_error;
				}
			}

			if (0 > run_command(cmd, 0)) {
				printf("MMC set device err\n");
				goto ret_error;
			}

			if (0 > get_device("mmc", simple_itoa(dev), &desc)) {
				printf("MMC get device err\n");
				goto ret_error;
			}

			memset(cmd, 0x0, sizeof(cmd));

			if (fs_type == UPDATE_SDCARD_FS_2NDBOOT ||
			    fs_type == UPDATE_SDCARD_FS_BOOT) {
				p = sprintf(cmd, "mmc write ");
				l = sprintf(&cmd[p], "0x%x 0x%llx 0x%llx",
					    (unsigned int)addr,
					    start / 512 ,
					    length / 512);
				p += l;
				cmd[p] = 0;

			} else if (fs_type & UPDATE_SDCARD_FS_MASK) {
				if (sdcard_mmc_check_part_table(desc, fp) > 0) {
					struct update_sdcard_part *fp_1 = fp;
					int j, cnt = 0;
					uint64_t part_start[DEV_PART_MAX];
					uint64_t part_length[DEV_PART_MAX];
					char args[1024];

					printf("Warn : [%s]", partition_name);
					printf("make new partitions ....\n");

					for (j = i; j < DEV_PART_MAX; j++,
					     fp_1++) {
						if (!strcmp(fp_1->device, ""))
							break;
						part_start[cnt] = fp_1->start;
						part_length[cnt] = fp_1->length;
						cnt++;
					}

					l = sprintf(args, "fdisk %d %d:",
								dev, cnt);
					p = l;

					for (j = 0; j < cnt; j++) {
						l = sprintf(&args[p],
							    " 0x%llx:0x%llx",
							    part_start[j],
							    part_length[j]);
						p += l;
					}

					if (p >= sizeof(args)) {
						printf("cmd stack overflow : ");
						printf("stack %ld, cmd %d **\n",
						       sizeof(args), p);
						while (1)
							;
					}

					args[p] = 0;
					printf("%s\n", args);

					if (0 > run_command(args, 0))
						printf("fdisk : %s\n", "FAIL");
					else
						printf("fdisk : %s\n", "DONE");
				}

				p = sprintf(cmd, "ext4_img_write %d %x %d %x",
					    dev, (unsigned int)addr, part_num,
					    (unsigned int)length);
			}

			if (0 > run_command(cmd, 0))
				printf("Flash : %s - %s\n", file_name, "FAIL");
			else
				printf("Flash : %s - %s\n", file_name, "DONE");
		}
	}
	printf("sd recovery end\n\n");
	return res;

ret_error:
	return -1;
}

U_BOOT_CMD(
	sd_recovery,	5,	1,	do_update_sdcard,
	"sd_recovery - Image Update from SDCard\n",
	"sd_recovery <interface> [<dev[:part]>] <addr> <filename>\n"
	"  ex> update_sdcard mmc 0:1 48000000 partmap.txt\n"
	"    - interface : mmc\n"
	"    - dev       : mmc channel\n"
	"    - part      : partition number\n"
	"    - addr      : image load address\n"
	"    - filename  : partition map file\n"
);
