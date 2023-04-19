#include <common.h>
#include <movi.h>
#include <asm/io.h>
#include <mmc.h>

extern raw_area_t raw_area_control;

typedef u32 (*copy_sd_mmc_to_mem) \
	(u32 start_block, u32 block_count, u32* dest_addr);

typedef u32 (*copy_emmc_to_mem) \
	(u32 block_size, u32 *buffer);

typedef u32 (*copy_emmc441_to_mem) \
	(u32 block_size, void *buffer);

typedef u32 (*emmc441_endboot_op)();


/* 同 emmc441_uboot_copy 解释 */
void movi_uboot_copy(void)
{
	copy_sd_mmc_to_mem copy_bl2 = (copy_sd_mmc_to_mem)*(u32 *)(0x02020030);
	copy_bl2(MOVI_UBOOT_POS, MOVI_UBOOT_BLKCNT, CFG_PHY_UBOOT_BASE);//mj
}

void emmc_uboot_copy(void)
{
	int i;
	char *ptemp;
	
    copy_emmc_to_mem copy_bl2 = (copy_emmc_to_mem)*(u32 *)(0x0202003c);

	/* for secure_bl1 (16KB) */
	copy_bl2(MOVI_UBOOT_BLKCNT, CFG_PHY_UBOOT_BASE); //mj
}


/* 从 eMMC441 中将uboot拷贝至 RAM 中， start.S 中使用 */
void emmc441_uboot_copy(void)
{
    /* 拷贝函数 MSH_ReadFromFIFO_eMMC 是 BL1 提供的，
     * BL1 将该函数的地址放在 0x02020044 位置处，这样uboot中就可以
     * 使用了。关于这部分介绍，参考:
     * Android_Exynos4412_iROM_Secure_Booting_Guide */
	copy_emmc441_to_mem copy_bl2 = \
    (copy_emmc441_to_mem)*(u32 *)(0x02020044);	//MSH_ReadFromFIFO_eMMC
	emmc441_endboot_op end_bootop = \
    (emmc441_endboot_op)*(u32 *)(0x02020048);	//MSH_EndBootOp_eMMC

    /* 拷贝指定的块数，放到指定物理内存中 */
	copy_bl2(/*0x8,*/ MOVI_UBOOT_BLKCNT, CFG_PHY_UBOOT_BASE); //mj

    /* 结束 eMMC4.4 启动模式 */
	/* stop bootop */
	end_bootop();
}


void movi_write_env(ulong addr)
{
	movi_write(raw_area_control.image[2].start_blk, 
		raw_area_control.image[2].used_blk, addr);
}

void movi_read_env(ulong addr)
{
	movi_read(raw_area_control.image[2].start_blk,
		raw_area_control.image[2].used_blk, addr);
}

void movi_write_bl1(ulong addr)
{
	int i;
	ulong checksum;
	ulong src;
	ulong tmp;

	src = addr;
	
	for(i = 0, checksum = 0;i < (14 * 1024) - 4;i++)
	{
		checksum += *(u8*)addr++;
	}

	tmp = *(ulong*)addr;
	*(ulong*)addr = checksum;
			
	movi_write(raw_area_control.image[1].start_blk, 
		raw_area_control.image[1].used_blk, src);

	*(ulong*)addr = tmp;
}

void movi_calc_checksum_bl1(ulong addr)
{
	int i;
	ulong checksum;
	ulong src;
	src = addr; /* from BL2 */
	
	for(i = 0, checksum = 0;i < (14 * 1024) - 4;i++)
	{
		checksum += *(u8*)addr++;
	}

	*(ulong*)addr = checksum;
}


