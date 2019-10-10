/*
 * (C) Copyright 2016 Nexell
 * Youngbok, Park <ybpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <mmc.h>
#include <asm/sections.h>
#include <part.h>
#include <image-sparse.h>
#include <sparse_format.h>

DECLARE_GLOBAL_DATA_PTR;

#define MMC_BLOCK_SIZE		(512)
#define SPARSE_FILL_BUF_SIZE (2 * 1024 * 1024)
#define WRITE_SECTOR            65536   /* 32 MB */

static lbaint_t mmc_sparse_write(struct sparse_storage *info,
		lbaint_t blk, lbaint_t blkcnt, const void *buffer)
{
	block_dev_desc_t *dev_desc = (block_dev_desc_t *) info->priv;
	ulong ret = 0;
	int fill_buf_num_blks, cnt;

	if ((unsigned long)buffer & (CONFIG_SYS_CACHELINE_SIZE - 1)) {

		fill_buf_num_blks = SPARSE_FILL_BUF_SIZE / info->blksz;

		while (blkcnt) {

			if (blkcnt > fill_buf_num_blks)
				cnt = fill_buf_num_blks;
			else
				cnt = blkcnt;

			ret += dev_desc->block_write(dev_desc->dev, blk, cnt, buffer);

			blk += cnt;
			blkcnt -= cnt;
			buffer = (void *)((unsigned long)buffer + cnt * info->blksz);

		}

	} else {
		ret = dev_desc->block_write(dev_desc->dev, blk, blkcnt, buffer);
	}

	return ret;
}

static lbaint_t mmc_sparse_reserve(struct sparse_storage *info,
        lbaint_t blk, lbaint_t blkcnt)
{
	return blkcnt;
}

int write_raw_chunk(char *data, unsigned int sector, unsigned int sector_size)
{
	char run_cmd[128] = {0, };
	unsigned char *tmp_align;
	char *ptr;
	int write_size;
	int write_sector_size;
	int remaining_sector_size;
	unsigned int write_sector;
	int ret;
	bool big = false;
	unsigned long fastboot_buf_end =
		(unsigned long)CONFIG_FASTBOOT_BUF_ADDR +
		(unsigned long)CONFIG_FASTBOOT_BUF_SIZE;

	tmp_align = (unsigned char *)(fastboot_buf_end & 0xffffffff);
	ptr = data;
	remaining_sector_size = sector_size;
	write_sector = sector;

	if (sector_size > WRITE_SECTOR) {
		big = true;
		debug("sector size ===> %d\n", sector_size);
	}

	while (remaining_sector_size > 0) {
		if (remaining_sector_size >= WRITE_SECTOR) {
			write_size = WRITE_SECTOR * MMC_BLOCK_SIZE;
			write_sector_size = WRITE_SECTOR;
		} else {
			write_size = remaining_sector_size *
				MMC_BLOCK_SIZE;
			write_sector_size = remaining_sector_size;
		}

		if (big)
			debug("ptr %p, write_sector %d, write_sector_size %d,\
				write_size 0x%x, remaining_sector_size %d\n",
				ptr, write_sector, write_sector_size,
				write_size, remaining_sector_size);

		memcpy(tmp_align, ptr, write_size);
		snprintf(run_cmd, sizeof(run_cmd), "mmc write 0x%x 0x%x 0x%x",
			(int)((ulong)tmp_align),
			write_sector, write_sector_size);
		ret = run_command(run_cmd, 0);
		if (ret != 0) {
			printf("failed to run_command %s\n", run_cmd);
			return ret;
		}

		remaining_sector_size -= write_sector_size;
		write_sector += write_sector_size;
		ptr += write_size;
	}

	return 0;
}

int do_compressed_ext4_write(cmd_tbl_t *cmdtp, int flag, int argc,
						char *const argv[])
{
	block_dev_desc_t *desc;
	uint64_t dst_addr = 0, mem_len = 0;
	unsigned int mem_addr = 0;
	unsigned char *p;
	char cmd[32];
	lbaint_t blk, cnt;
	int ret, dev;
	char *downloadbytes_str;
	unsigned int fb_download_bytes;

	if (5 > argc)
		goto usage;

	ret = get_device("mmc", argv[1], &desc);
	if (0 > ret) {
		printf("** Not find device mmc.%s **\n", argv[1]);
		return 1;
	}
	dev = simple_strtoul(argv[1], NULL, 10);
	sprintf(cmd, "mmc dev %d", dev);
	if (0 > run_command(cmd, 0))	/* mmc device */
		return -1;

	mem_addr = simple_strtoul(argv[2], NULL, 16);
	dst_addr = simple_strtoull(argv[3], NULL, 10);
	mem_len  = simple_strtoull(argv[4], NULL, 16);

	p   = (unsigned char *)((ulong)mem_addr);
	blk = (dst_addr/MMC_BLOCK_SIZE);
	cnt = (mem_len/MMC_BLOCK_SIZE) +
		((mem_len & (MMC_BLOCK_SIZE-1)) ? 1 : 0);

	flush_dcache_all();

	uint64_t parts[32][2] = { {0, 0}, };
	uint64_t part_len = 0;
	int partno = (int)dst_addr;
	int num = 0;

	if (0 > get_part_table(desc, parts, &num))
			return 1;

	if (partno > num || 1 > partno)  {
		printf("Invalid mmc.%d partition number %d (1 ~ %d)\n",
		       dev, partno, num);
		return 1;
	}

	dst_addr = parts[partno-1][0];
	part_len = parts[partno-1][1];
	blk = (dst_addr/MMC_BLOCK_SIZE);

	if (is_sparse_image((void *)p)) {
		printf("sparse image fusing\n");
		struct sparse_storage sparse;
		disk_partition_t info;

		if(num > 4 && partno >= 4)
			partno += 1;

		if (get_partition_info(desc, partno, &info)) {
			printf("Bad partition index:%d \n",	partno);
		}
		sparse.blksz = info.blksz;
		sparse.start = info.start;
		sparse.size = info.size;
		sparse.write = mmc_sparse_write;
		sparse.reserve = mmc_sparse_reserve;

		printf("Flashing sparse image at offset " LBAFU "\n",
               info.start);
		sparse.priv = desc;

		downloadbytes_str = getenv("fb_downloadbytes");
		if(downloadbytes_str) {
			fb_download_bytes = (unsigned int) simple_strtoul(downloadbytes_str,
					NULL, 10);
		}
		printf("fb_download_bytes = %d \n", fb_download_bytes);
		write_sparse_image(&sparse, argv[1], p, fb_download_bytes);
		return 1;
	}
	goto do_write;

do_write:
	if (!blk) {
		printf("Fail: start %d block(0x%llx) is in MBR zone (0x200)\n",
		       (int)blk, dst_addr);
		return -1;
	}

	printf("write mmc.%d = 0x%llx(0x%x) ~ 0x%llx(0x%x): ",
		dev, dst_addr, (unsigned int)blk, mem_len, (unsigned int)cnt);

	ret = mmc_bwrite(dev, blk, cnt, (void const *)p);

	printf("%s\n", ret ? "Done" : "Fail");
	return ret;

usage:
	cmd_usage(cmdtp);
	return 1;
}

U_BOOT_CMD(
	ext4_img_write, CONFIG_SYS_MAXARGS, 1,	do_compressed_ext4_write,
	"Compressed ext4 image write",
	"img_write <dev no> 'mem' 'part no' 'length'\n"
	"    - update partition image 'length' on 'mem' to mmc 'part no'.\n\n"
	"Note.\n"
	"    - All numeric parameters are assumed to be hex.\n"
);

