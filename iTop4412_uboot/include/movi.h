#ifndef __MOVI_H__
#define __MOVI_H__


/* raw_area 幻数，主要是开机时的首次初始化 */
#define MAGIC_NUMBER_MOVI	(0x24564236)

// 未使用
#define SS_SIZE			    (16 * 1024)

// eFUSE_SIZE 这里是512字节，正好一个扇区，就是SD卡最前预留的一个扇区
#define eFUSE_SIZE		    (1 * 512)	// 512 Byte eFuse, 512 Byte reserved

/* 扇区大小 */
#define MOVI_BLKSIZE		(1 << 9) /* 512 bytes */

// FWBL1_SIZE 这里是8KB， 但在新的手册中已经是 15KB了
#define FWBL1_SIZE		    (8 * 1024) //IROM BL1 SIZE 8KB
#define BL2_SIZE		    (16 * 1024)//uboot BL1 16KB

/* partition information */
/* 预留的 uboot区域，实际是 328KB + 156KB */
#define PART_SIZE_UBOOT		(495 * 1024)
/* 预留的 kernel 区域， 共预留了 6MB 区域 */
#define PART_SIZE_KERNEL	(6 * 1024 * 1024)	//modify by cym 20140217

/* rootfs 区域 */
#define PART_SIZE_ROOTFS	(2 * 1024 * 1024)//  2M
/* raw_area 总预留的区域大小 */
#define RAW_AREA_SIZE		(16 * 1024 * 1024)// 16MB

/* 未使用， raw_area 总预留区域 */
#define MOVI_RAW_BLKCNT		(RAW_AREA_SIZE / MOVI_BLKSIZE)	/* 16MB */
/* fwbl1 的扇区数量，预留了 8KB */
#define MOVI_FWBL1_BLKCNT	(FWBL1_SIZE / MOVI_BLKSIZE)	/* FWBL1:8KB */
/* bl2 的扇区数量，预留了 16KB */
#define MOVI_BL2_BLKCNT		(BL2_SIZE / MOVI_BLKSIZE)	/* BL2:16KB */
/* 环境变量预留区域 */
#define MOVI_ENV_BLKCNT		(CONFIG_ENV_SIZE / MOVI_BLKSIZE)	/* ENV:16KB */
#define MOVI_UBOOT_BLKCNT	(PART_SIZE_UBOOT / MOVI_BLKSIZE)/* UBOOT:512KB */
#define MOVI_ZIMAGE_BLKCNT	(PART_SIZE_KERNEL / MOVI_BLKSIZE)/* 6MB */
/* 未使用，直接在代码里写死了 */
#define ENV_START_BLOCK		(544*1024)/MOVI_BLKSIZE

#define MOVI_UBOOT_POS		((eFUSE_SIZE / MOVI_BLKSIZE) + MOVI_FWBL1_BLKCNT + MOVI_BL2_BLKCNT)

#define MOVI_ROOTFS_BLKCNT	(PART_SIZE_ROOTFS / MOVI_BLKSIZE)

/*
 * start_blk: start block number for image
 * used_blk: blocks occupied by image
 * size: image size in bytes
 * attribute: attributes of image
 *            0x1: u-boot parted (BL1)
 *            0x2: u-boot (BL2)
 *            0x4: kernel
 *            0x8: root file system
 *            0x10: environment area
 *            0x20: reserved
 * description: description for image
 * by scsuh
 */
typedef struct member {
	uint start_blk;
	uint used_blk;
	uint size;
	uint attribute; /* attribute of image */
	char description[16];
} member_t; /* 32 bytes */

/*
 * magic_number: 0x24564236
 * start_blk: start block number for raw area
 * total_blk: total block number of card
 * next_raw_area: add next raw_area structure
 * description: description for raw_area
 * image: several image that is controlled by raw_area structure
 * by scsuh
 */
typedef struct raw_area {
	uint magic_number; /* to identify itself */
	uint start_blk; /* compare with PT on coherency test */
	uint total_blk;
	uint next_raw_area; /* should be sector number */
	char description[16];
	member_t image[15];
} raw_area_t; /* 512 bytes */

#endif /*__MOVI_H__*/
