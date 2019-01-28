/*
 * (C) Copyright 2019 Nexell
 * Sungwoo, Park <swpark@nexell.co.kr>
 *
 * SPDX-License-Identifier:      GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <part.h>
#include <errno.h>

/**
 * Nexell Recovery Header
 * -----------------------------------------------------------------
 * Category | Field         | Size | Value  | Datatype | Description
 * -----------------------------------------------------------------
 * HEADER   | TAG           | 4    | 'NIDH' | chars    |
 *          | Image Count   | 4    |        | u32      |
 *          | Make Part?    | 4    |        | bool     | run fdisk?
 * Image    | TAG           | 4    | 'IMGH' | chars    |
 *          | Type          | 4    |        | u32      | NX_IMAGE_TYPE...
 *          | Dest Offset   | 8    |        | u64      |
 *          | Dest Size     | 8    |        | u64      |
 *          | Source Offset | 8    |        | u64      |
 *          | Source Size   | 8    |        | u64      |
 *-----------------------------------------------------------------
 */
#define MAX_IMAGE_NUM	16
#define NX_ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))

enum {
	NX_IMAGE_TYPE_NONE = 0,
	NX_IMAGE_TYPE_RAW,
	NX_IMAGE_TYPE_RAW_PART,
	NX_IMAGE_TYPE_EXT4SPARSE,
	NX_IMAGE_TYPE_MAX,
};

struct nx_image {
	u32 type;
	u64 dest_offset;
	u64 dest_size;
	u64 src_offset;
	u64 src_size;
};

struct nx_recovery {
	/* header info */
	u32 img_count;
	bool do_fdisk;
	struct nx_image images[MAX_IMAGE_NUM];
	/* info for runtime */
	char *buf;
	u32 buf_size;
	block_dev_desc_t *sdesc;
	block_dev_desc_t *tdesc;
	int sdev;
	int tdev;
};

static struct nx_recovery nx_recovery;

static void dump_nx_recovery(struct nx_recovery *h)
{
	int i;
	struct nx_image *pimg;

	printf("image count: %d\n", h->img_count);
	printf("do_fdisk? %s\n", h->do_fdisk ? "yes": "no" );

	pimg = &h->images[0];
	for (i = 0; i < h->img_count; i++) {
		printf("IMAGE[%d] -->\n", i);
		printf("\ttype %s\n",
		       pimg->type == NX_IMAGE_TYPE_RAW ? "raw" :
		       pimg->type == NX_IMAGE_TYPE_RAW_PART ? "raw_part" :
		       "ext4sparse");
		printf("\tdest_offset 0x%llx\n", pimg->dest_offset);
		printf("\tdest_size %lld\n", pimg->dest_size);
		printf("\tsrc_offset 0x%llx\n", pimg->src_offset);
		printf("\tsrc_size %lld\n", pimg->src_size);

		pimg++;
	}
}

static int read_header(struct nx_recovery *h)
{
	int dev = h->sdev;
	block_dev_desc_t *desc = h->sdesc;
	char *addr = h->buf;
	u32 *p32;
	u64 *p64;
	int i;
	struct nx_image *pimg;

	if (desc->block_read(dev, 0, 1, addr) != 1) {
		printf("%s: Read error for header\n", __func__);
		return 1;
	}

	/* check header tag */
	p32 = (u32 *)addr;
	if (*p32 != 0x4844494e) {
		printf("Invalid Header TAG(0x%x)\n", *p32);
		return 1;
	}
	addr += sizeof(u32);

	/* image count */
	p32 = (u32 *)addr;
	h->img_count = *p32;
	addr += sizeof(u32);

	/* do fdisk? */
	p32 = (u32 *)addr;
	h->do_fdisk = *p32;
	addr += sizeof(u32);

	/* reserved */
	addr += sizeof(u32);

	if (h->img_count > MAX_IMAGE_NUM) {
		printf("image count is too big(%d)!!!\n", h->img_count);
		printf("use under %d count\n", MAX_IMAGE_NUM);
		return 1;
	}

	pimg = &h->images[0];
	for (i = 0; i < h->img_count; i++) {
		/* check image tag */
		p32 = (u32 *)addr;
		if (*p32 != 0x48474d49) {
			printf("Invalid Image TAG(0x%x)\n", *p32);
			return 1;
		}
		addr += sizeof(u32);
		/* image type */
		p32 = (u32 *)addr;
		pimg->type = *p32;
		if (pimg->type == NX_IMAGE_TYPE_NONE ||
		    pimg->type >= NX_IMAGE_TYPE_MAX) {
			printf("Invalid image type(%d)\n", pimg->type);
			return 1;
		}
		addr += sizeof(u32);
		/* dest offset */
		p64 = (u64 *)addr;
		pimg->dest_offset = *p64;
		addr += sizeof(u64);
		/* dest size */
		p64 = (u64 *)addr;
		pimg->dest_size = *p64;
		addr += sizeof(u64);
		/* src offset */
		p64 = (u64 *)addr;
		pimg->src_offset = *p64;
		addr += sizeof(u64);
		/* src size */
		p64 = (u64 *)addr;
		pimg->src_size = *p64;
		addr += sizeof(u64);

		pimg++;
	}

	dump_nx_recovery(h);

	return 0;
}

static int read_data(block_dev_desc_t *desc, int dev, u64 offset,
		     u64 size, char *buf)
{
	lbaint_t blk_num, blk_cnt;

	blk_num = offset / 512;
	blk_cnt = NX_ALIGN(size, 512) / 512;

	printf("read blk 0x" LBAF ", cnt 0x" LBAF "\n", blk_num, blk_cnt);

	if (desc->block_read(dev, blk_num, blk_cnt, buf) != blk_cnt)
		return 1;

	return 0;
}

static int write_data(block_dev_desc_t *desc, int dev, u64 offset,
		      u64 size, char *buf)
{
	lbaint_t blk_num, blk_cnt;

	blk_num = offset / 512;
	blk_cnt = NX_ALIGN(size, 512) / 512;

	printf("write blk 0x" LBAF ", cnt 0x" LBAF "\n", blk_num, blk_cnt);

	if (desc->block_write(dev, blk_num, blk_cnt, buf) != blk_cnt)
		return 1;

	return 0;
}

/* common/cmd_mmc_fdisk.c */
#define MAX_PART_TABLE	15
extern int mmc_make_part_table(block_dev_desc_t *desc,
			       uint64_t (*parts)[2],
			       int part_num,
			       unsigned int part_type);

static int make_partition(struct nx_recovery *h)
{
	struct nx_image *pimg;
	int part_num;
	int i;
	uint64_t parts[MAX_PART_TABLE][2];

	pimg = &h->images[0];
	part_num = 0;
	for (i = 0; i < h->img_count; i++) {
		if (pimg->type == NX_IMAGE_TYPE_RAW_PART ||
		    pimg->type == NX_IMAGE_TYPE_EXT4SPARSE) {
			parts[part_num][0] = pimg->dest_offset; /* start */
			parts[part_num][1] = NX_ALIGN(pimg->dest_size, 512);
			part_num++;
		}
		pimg++;
	}

	if (part_num > 0)
		mmc_make_part_table(h->tdesc, parts, part_num, PART_TYPE_DOS);

	return 0;
}

static int update_raw(struct nx_recovery *h, struct nx_image *img)
{
	char *buf = h->buf;
	u32 bsize = h->buf_size;
	u64 ssize = img->src_size;
	u64 soffset = img->src_offset;
	u64 doffset = img->dest_offset;
	block_dev_desc_t *sdesc = h->sdesc;
	block_dev_desc_t *tdesc = h->tdesc;
	int sdev = h->sdev;
	int tdev = h->tdev;
	u32 rw_size;
	u64 done;
	int err;

	if (img->src_offset == 0 || img->src_size == 0)
		return 0;

	done = 0;
	while (done < ssize) {
		rw_size = (ssize - done) > bsize ? bsize : ssize;

		err = read_data(sdesc, sdev, soffset + done, rw_size, buf);
		if (err) {
			printf("failed to read: offset(0x%llx), size(%d)\n",
			       soffset + done, rw_size);
			return err;
		}

		err = write_data(tdesc, tdev, doffset + done, rw_size, buf);
		if (err) {
			printf("failed to write: offset(0x%llx), size(%d)\n",
			       doffset + done, rw_size);
			return err;
		}

		done += rw_size;
	}

	return 0;
}

struct ext4_sparse_header {
	unsigned int magic;
	unsigned short major;
	unsigned short minor;
	unsigned short file_header_size;
	unsigned short chunk_header_size;
	unsigned int block_size;
	unsigned int total_blocks;
	unsigned int total_chunks;
	unsigned int crc32;
};

struct ext4_chunk_header {
	unsigned short type;
	unsigned short reserved;
	unsigned int chunk_size;
	unsigned int total_size;
};

#define EXT4_FILE_HEADER_MAGIC	0xED26FF3A
#define EXT4_FILE_HEADER_MAJOR	0x0001
#define EXT4_FILE_HEADER_MINOR	0x0000
#define EXT4_FILE_BLOCK_SIZE	0x1000

#define EXT4_FILE_HEADER_SIZE	(sizeof(struct ext4_sparse_header))
#define EXT4_CHUNK_HEADER_SIZE	(sizeof(struct ext4_chunk_header))

#define EXT4_CHUNK_TYPE_RAW		0xCAC1
#define EXT4_CHUNK_TYPE_FILL		0xCAC2
#define EXT4_CHUNK_TYPE_NONE		0xCAC3

static int get_ext4sparse_header(char *buf, struct ext4_sparse_header *sh)
{
	struct ext4_sparse_header *h = (struct ext4_sparse_header *)buf;

	if (h->magic != EXT4_FILE_HEADER_MAGIC) {
		printf("Invalid magic 0x%x\n", h->magic);
		return -1;
	}

	if (h->major != EXT4_FILE_HEADER_MAJOR) {
		printf("Invalid major version  0x%x\n", h->major);
		return -1;
	}

	if (h->file_header_size != EXT4_FILE_HEADER_SIZE) {
		printf("Invalid file header size 0x%x\n", h->chunk_header_size);
		return -1;
	}

	memcpy(sh, buf, sizeof(*sh));
	return 0;
}

/* common/cmd_ext4_img_write.c */
extern int write_raw_chunk(char *data, unsigned int sector,
			   unsigned int sector_size);
static int update_ext4sparse(struct nx_recovery *h, struct nx_image *img)
{
	char *buf;
	u32 bsize = h->buf_size;
	u64 ssize = img->src_size;
	u64 soffset = img->src_offset;
	u64 doffset = img->dest_offset;
	block_dev_desc_t *sdesc = h->sdesc;
	int sdev = h->sdev;
	u32 rw_size;
	u64 done;
	u64 wdone;
	int err;
	struct ext4_sparse_header e4sh;
	bool header_checked = false;
	u32 write_chunks = 0;
	struct ext4_chunk_header *ch;

	done = 0;
	wdone = 0;
	while (done < ssize) {
		rw_size = (ssize - done) > bsize ? bsize : ssize;
		buf = h->buf;

		err = read_data(sdesc, sdev, soffset + done, rw_size, buf);
		if (err) {
			printf("failed to read: offset(0x%llx), size(%d)\n",
			       soffset + done, rw_size);
			return err;
		}

		if (!header_checked) {
			err = get_ext4sparse_header(buf, &e4sh);
			if (err) {
				printf("Invalid ext4 sparse header\n");
				return err;
			}
			header_checked = true;
			buf += EXT4_FILE_HEADER_SIZE;
		}

		while (true) {
			ch = (struct ext4_chunk_header *)buf;
			buf += ch->total_size;

			if ((buf - h->buf) >= bsize) {
				/* split */
				rw_size = (char *)ch - h->buf;
				break;
			} else {
				char *wptr =
					(char *)ch + EXT4_CHUNK_HEADER_SIZE;
				u32 wsize = ch->chunk_size * e4sh.block_size;

				switch (ch->type) {
				case EXT4_CHUNK_TYPE_RAW:
					err = write_raw_chunk(wptr,
							(doffset + wdone)/512,
							wsize/512);
					if (err) {
						printf("failed to write\n");
						return err;
					}
					break;
				case EXT4_CHUNK_TYPE_FILL:
					/* printf("*** fill_chunk ***\n"); */
					break;
				case EXT4_CHUNK_TYPE_NONE:
					/* printf("*** none_chunk ***\n"); */
					break;
				default:
					/* printf("*** unknown chunk ***\n"); */
					break;
				}

				wdone += wsize;
				write_chunks++;
			}

			if (write_chunks >= e4sh.total_chunks)
				break;
		}

		done += rw_size;
		if (write_chunks >= e4sh.total_chunks)
			break;
	}

	return 0;
}

static int update_image(struct nx_recovery *h, struct nx_image *img)
{
	int err = 0;

	switch (img->type) {
	case NX_IMAGE_TYPE_RAW:
	case NX_IMAGE_TYPE_RAW_PART:
		err = update_raw(h, img);
		break;
	case NX_IMAGE_TYPE_EXT4SPARSE:
		err = update_ext4sparse(h, img);
		break;
	}

	return err;
}

static int do_update(struct nx_recovery *h)
{
	int i;
	struct nx_image *pimg;
	int err;

	if (h->do_fdisk) {
		err = make_partition(h);
		if (err) {
			printf("failed to make_partition: %d\n", err);
			return err;
		}
	}

	pimg = &h->images[0];
	for (i = 0; i < h->img_count; i++) {
		err = update_image(h, pimg);
		if (err) {
			printf("failed to update image(%d)\n", i);
			return err;
		}
		pimg++;
	}

	return 0;
}

static int do_nxrecovery(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	const char *src_dev_name, *target_dev_name;
	u32 src_dev_num, target_dev_num;
	block_dev_desc_t *src_dev, *target_dev;
	int err;
	struct nx_recovery *h = &nx_recovery;

	if (argc < 5)
		return CMD_RET_USAGE;

	if (argc < 7) {
		h->buf = (char *)CONFIG_FASTBOOT_BUF_ADDR;
		h->buf_size = CONFIG_FASTBOOT_BUF_SIZE;
	} else {
		h->buf = (char *)simple_strtoul(argv[5], NULL, 16);
		h->buf_size = simple_strtoul(argv[6], NULL, 16);
	}

	src_dev_name = argv[1];
	src_dev_num = simple_strtoul(argv[2], NULL, 10);
	target_dev_name = argv[3];
	target_dev_num = simple_strtoul(argv[4], NULL, 10);

	src_dev = get_dev(src_dev_name, src_dev_num);
	if (!src_dev) {
		printf("Can't get source device(%s:%d)\n",
		       src_dev_name, src_dev_num);
		return 1;
	}
	target_dev = get_dev(target_dev_name, target_dev_num);
	if (!target_dev) {
		printf("Can't get target device(%s:%d)\n",
		       target_dev_name, target_dev_num);
		return 1;
	}
	if (src_dev == target_dev) {
		printf("Error: src(%s:%d) must differ with target(%s:%d)\n",
		       src_dev_name, src_dev_num,
		       target_dev_name, target_dev_num);
		return 1;
	}

	printf("src_dev(%s:%d), target_dev(%s:%d), addr(%p:0x%x)\n",
	       src_dev_name, src_dev_num, target_dev_name, target_dev_num,
	       h->buf, h->buf_size);

	h->sdesc = src_dev;
	h->tdesc = target_dev;
	h->sdev = src_dev_num;
	h->tdev = target_dev_num;

	err = read_header(h);
	if (err)
		return err;

	err = do_update(h);
	if (err)
		return err;

	printf("Update done!!!\n");
	printf("Change boot mode to target device and reboot\n");

	return 0;
}

U_BOOT_CMD(
	nxrecovery, 7, 0, do_nxrecovery,
	"Nexell recovery tool",
	"<src_dev_inf> <src_dev_num> <target_dev_inf> <target_dev_num> [<buf_addr> <buf_size>]\n"
	"  recovery from source device to target device\n"
	"\tsrc_dev_inf - source device name (mmc, usb)\n"
	"\tsrc_dev_num - source device number (0, 1 ...)\n"
	"\ttarget_dev_inf - target device name (mmc, usb)\n"
	"\ttarget_dev_num - target device number (0, 1 ...)\n"
	"\t[optional]buf_addr - address of buffer by hex\n"
	"\t[optional]buf_size - size of buffer by hex\n"
	""
);
