// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2017 The Android Open Source Project
 */

#include <common.h>
#include <android_ab.h>
#include <android_bl_msg.h>
#include <memalign.h>
#include <u-boot/crc.h>
#include <errno.h>

/**
 * Compute the CRC-32 of the bootloader control struct.
 *
 * Only the bytes up to the crc32_le field are considered for the CRC-32
 * calculation.
 */
static uint32_t ab_control_compute_crc(struct andr_bl_control *abc)
{
	return crc32(0, (void *)abc, offsetof(typeof(*abc), crc32_le));
}

/**
 * Initialize andr_bl_control to the default value.
 *
 * It allows us to boot all slots in order from the first one. This value
 * should be used when the bootloader message is corrupted, but not when
 * a valid message indicates that all slots are unbootable.
 */
static void ab_control_default(struct andr_bl_control *abc)
{
	int i;
	const struct andr_slot_metadata metadata = {
		.priority = 15,
		.tries_remaining = 7,
		.successful_boot = 0,
		.verity_corrupted = 0,
		.reserved = 0
	};

        printf("[debug][suker][%s]\n", __func__);
        
	memcpy(abc->slot_suffix, "a\0\0\0", 4);
	abc->magic = ANDROID_BOOT_CTRL_MAGIC;
	abc->version = ANDROID_BOOT_CTRL_VERSION;
	abc->nb_slot = ANDROID_NUM_SLOTS;
	memset(abc->reserved0, 0, sizeof(abc->reserved0));
	for (i = 0; i < abc->nb_slot; ++i)
		abc->slot_info[i] = metadata;

	memset(abc->reserved1, 0, sizeof(abc->reserved1));
	abc->crc32_le = ab_control_compute_crc(abc);
}

/**
 * Load the boot_control struct from disk into newly allocated memory.
 *
 * This function allocates and returns an integer number of disk blocks,
 * based on the block size of the passed device to help performing a
 * read-modify-write operation on the boot_control struct.
 * The boot_control struct offset (2 KiB) must be a multiple of the device
 * block size, for simplicity.
 *
 * @param[in] dev_desc Device where to read the boot_control struct from
 * @param[in] part_info Partition in 'dev_desc' where to read from, normally
 *			the "misc" partition should be used
 * @param[out] pointer to pointer to andr_bl_control data
 * @return 0 on success and a negative on error
 */
static int ab_control_create_from_disk(struct block_dev_desc *dev_desc,
				       const disk_partition_t *part_info,
				       struct andr_bl_control **abc)
{
	ulong abc_offset, abc_blocks;

        unsigned int i;

	abc_offset = offsetof(struct andr_bl_msg_ab, slot_suffix);

	if (abc_offset % part_info->blksz) {
		printf("ANDROID: Boot control block not block aligned.\n");
		return -EINVAL;
	}

	abc_offset /= part_info->blksz;
	abc_blocks = DIV_ROUND_UP(sizeof(struct andr_bl_control),
				  part_info->blksz);

        printf("[debug][suker][%s] part_info->size = 0x%lx\n", __func__, part_info->size);
	if (abc_offset + abc_blocks > part_info->size) {
		printf("ANDROID: boot control partition too small. Need at");
		printf(" least %lu blocks but have %lu blocks.\n",
		       abc_offset + abc_blocks, part_info->size);
		return -EINVAL;
	}

	*abc = malloc_cache_aligned(abc_blocks * part_info->blksz);
	if (!*abc)
		return -ENOMEM;

	if (dev_desc->block_read(dev_desc->dev,
                      part_info->start + abc_offset, abc_blocks,
                      *abc) != abc_blocks) {
		printf("ANDROID: Could not read from boot control partition\n");
		free(*abc);
		return -EIO;
	}
	printf("ANDROID: Loaded ABC, %lu blocks\n", abc_blocks);
	return 0;
}

/**
 * Store the loaded boot_control block.
 *
 * Store back to the same location it was read from with
 * ab_control_create_from_misc().
 *
 * @param[in] dev_desc Device where we should write the boot_control struct
 * @param[in] part_info Partition on the 'dev_desc' where to write
 * @param[in] abc Pointer to the boot control struct and the extra bytes after
 *                it up to the nearest block boundary
 * @return 0 on success and a negative on error
 */
static int ab_control_store(struct block_dev_desc *dev_desc,
			    const disk_partition_t *part_info,
			    struct andr_bl_control *abc)
{
	ulong abc_offset, abc_blocks;

        printf("[debug][suker][%s]\n", __func__);
	abc_offset = offsetof(struct andr_bl_msg_ab, slot_suffix) /
		     part_info->blksz;
	abc_blocks = DIV_ROUND_UP(sizeof(struct andr_bl_control),
				  part_info->blksz);
	if (dev_desc->block_write(dev_desc->dev,
                       part_info->start + abc_offset, abc_blocks,
		       abc) != abc_blocks) {
		printf("ANDROID: Could not write back the misc partition\n");
		return -EIO;
	}
	return 0;
}

/**
 * Compare two slots.
 *
 * The function determines slot which is should we boot from among the two.
 *
 * @param[in] a The first bootable slot metadata
 * @param[in] b The second bootable slot metadata
 * @return Negative if the slot "a" is better, positive of the slot "b" is
 *         better or 0 if they are equally good.
 */
static int ab_compare_slots(const struct andr_slot_metadata *a,
			    const struct andr_slot_metadata *b)
{
        printf("[debug][suker][%s]\n", __func__);

	/* Higher priority is better */
	if (a->priority != b->priority)
		return b->priority - a->priority;

	/* Higher successful_boot value is better, in case of same priority */
	if (a->successful_boot != b->successful_boot)
		return b->successful_boot - a->successful_boot;

	/* Higher tries_remaining is better to ensure round-robin */
	if (a->tries_remaining != b->tries_remaining)
		return b->tries_remaining - a->tries_remaining;

	return 0;
}

int ab_select_slot(struct block_dev_desc *dev_desc, disk_partition_t *part_info)
{
	struct andr_bl_control *abc = NULL;
	u32 crc32_le;
	int slot, i, ret;
	bool store_needed = false;
	char slot_suffix[4];

        printf("[debug][suker][%s]\n", __func__);
	ret = ab_control_create_from_disk(dev_desc, part_info, &abc);
        printf("[debug][suker][%s] ret = %d\n", __func__, ret);
        printf("[debug][suker][%s] ----abc info---\n", __func__);
        printf("[debug][suker][%s] abc->slot_suffix  = %s\n",   __func__, abc->slot_suffix );
        printf("[debug][suker][%s] abc->magic        = 0x%x\n", __func__, abc->magic       );
        printf("[debug][suker][%s] abc->nb_slot      = %d\n",   __func__, abc->nb_slot     );
        printf("[debug][suker][%s] abc->crc32_le     = %d\n",   __func__, abc->crc32_le    );
        printf("[debug][suker][%s] ---------------\n", __func__, ret);

	if (!abc || ret < 0) {
		/*
		 * This condition represents an actual problem with the code or
		 * the board setup, like an invalid partition information.
		 * Signal a repair mode and do not try to boot from either slot.
		 */
		return ret;
	}

	crc32_le = ab_control_compute_crc(abc);
	if (abc->crc32_le != crc32_le) {
		printf("ANDROID: Invalid CRC-32 (expected %.8x, found %.8x), ",
		       crc32_le, abc->crc32_le);
		printf("re-initializing A/B metadata.\n");
		ab_control_default(abc);
		store_needed = true;
	}

	if (abc->magic != ANDROID_BOOT_CTRL_MAGIC) {
		printf("ANDROID: Unknown A/B metadata: %.8x\n", abc->magic);
		free(abc);
		return -ENODATA;
	}

	if (abc->version > ANDROID_BOOT_CTRL_VERSION) {
		printf("ANDROID: Unsupported A/B metadata version: %.8x\n",
		       abc->version);
		free(abc);
		return -ENODATA;
	}

	/*
	 * At this point a valid boot control metadata is stored in abc,
	 * followed by other reserved data in the same block. We select a with
	 * the higher priority slot that
	 *  - is not marked as corrupted and
	 *  - either has tries_remaining > 0 or successful_boot is true.
	 * If the selected slot has a false successful_boot, we also decrement
	 * the tries_remaining until it eventually becomes unbootable because
	 * tries_remaining reaches 0. This mechanism produces a bootloader
	 * induced rollback, typically right after a failed update.
	 */

	/* Safety check: limit the number of slots. */
	if (abc->nb_slot > ARRAY_SIZE(abc->slot_info)) {
		abc->nb_slot = ARRAY_SIZE(abc->slot_info);
		store_needed = true;
	}

	slot = -1;
	for (i = 0; i < abc->nb_slot; ++i) {
		if (abc->slot_info[i].verity_corrupted ||
		    !abc->slot_info[i].tries_remaining) {
			printf("ANDROID: unbootable slot %d tries: %d, ",
				  i, abc->slot_info[i].tries_remaining);
			printf("corrupt: %d\n",
				  abc->slot_info[i].verity_corrupted);
			continue;
		}
		printf("ANDROID: bootable slot %d pri: %d, tries: %d, ",
			  i, abc->slot_info[i].priority,
			  abc->slot_info[i].tries_remaining);
		printf("corrupt: %d, successful: %d\n",
			  abc->slot_info[i].verity_corrupted,
			  abc->slot_info[i].successful_boot);

		if (slot < 0 ||
		    ab_compare_slots(&abc->slot_info[i],
				     &abc->slot_info[slot]) < 0) {
			slot = i;
		}
	}

	if (slot >= 0 && !abc->slot_info[slot].successful_boot) {
		printf("ANDROID: Attempting slot %c, tries remaining %d\n",
		       ANDROID_BOOT_SLOT_NAME(slot),
		       abc->slot_info[slot].tries_remaining);
		abc->slot_info[slot].tries_remaining--;
		store_needed = true;
	}

	if (slot >= 0) {
		/*
		 * Legacy user-space requires this field to be set in the BCB.
		 * Newer releases load this slot suffix from the command line
		 * or the device tree.
		 */
		memset(slot_suffix, 0, sizeof(slot_suffix));
		slot_suffix[0] = ANDROID_BOOT_SLOT_NAME(slot);
		if (memcmp(abc->slot_suffix, slot_suffix,
			   sizeof(slot_suffix))) {
			memcpy(abc->slot_suffix, slot_suffix,
			       sizeof(slot_suffix));
			store_needed = true;
		}
	}

	if (store_needed) {
		abc->crc32_le = ab_control_compute_crc(abc);
		ab_control_store(dev_desc, part_info, abc);
	}
	free(abc);

        printf("[debug][suker][%s] end slot = %d\n", __func__,slot);
	if (slot < 0)
		return -EINVAL;

	return slot;
}
