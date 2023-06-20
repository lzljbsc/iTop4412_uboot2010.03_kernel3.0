
#include <common.h>
#include <s5pc210.h>
#include <asm/io.h>

int check_bootmode(void);
unsigned int OmPin;
int boot_mode;
extern char *CORE_NUM_STR;

DECLARE_GLOBAL_DATA_PTR;
extern int nr_dram_banks;

unsigned int dmc_density = 0xFFFFFFFF;

static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n" "bne 1b":"=r" (loops):"0"(loops));
}

int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

    /* 设置 MACH_TYPE, 该参数与内核中的匹配 */
	gd->bd->bi_arch_number = MACH_TYPE;

    /* 设置启动参数存放位置，位于物理内存起始地址偏移 0x100(256字节) 处 */
    /* 这里有一个偏移, 而不是内存起始位置, 将会把这个地址做为参数传给内核 */
    /* 这个位置需要传递给内核使用，不能被覆盖
     * 内核中建议放在16KiB处，其实放哪里无所谓，只要不被覆盖即可 */
	gd->bd->bi_boot_params = (PHYS_SDRAM_1+0x100);

	return 0;
}

int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

    /* CHIP_ID_BASE 寄存器中的 [9:8]位 表示封装信息 
     * 0x0: SCP ; 0x2: POP */
	if(((*((volatile unsigned long *)CHIP_ID_BASE) & 0x300) >> 8) == 2){
	 	printf("POP type: ");
        /* dmc_density 是在 bl2 阶段设置的，在汇编里面 */
		if(dmc_density == 6){
			printf("POP_B\n");
			nr_dram_banks = 2;
			gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
			gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
			gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
			gd->bd->bi_dram[1].size = 0x4000000;//PHYS_SDRAM_2_SIZE;
			/*gd->bd->bi_dram[2].start = PHYS_SDRAM_3;
			gd->bd->bi_dram[2].size = PHYS_SDRAM_3_SIZE;
			gd->bd->bi_dram[3].start = PHYS_SDRAM_4;
			gd->bd->bi_dram[3].size = PHYS_SDRAM_4_SIZE;*/
		}
		else if(dmc_density == 5){
			printf("POP_A\n");
			nr_dram_banks = 2;
			gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
			gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
			gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
			gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
		}
		else{//ly
            printf("POP for C220\n");
            nr_dram_banks = 4;
            gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
            gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
            gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
            gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
            gd->bd->bi_dram[2].start = PHYS_SDRAM_3;
            gd->bd->bi_dram[2].size = PHYS_SDRAM_3_SIZE;
            gd->bd->bi_dram[3].start = PHYS_SDRAM_4;
            gd->bd->bi_dram[3].size = PHYS_SDRAM_4_SIZE;
		}
	} else{
        /* SCP的内存在uboot下是固定的，通过配置文件配置的 */
        /* 总共8个bank，每个bank 128MB， 共 1GB 
         * 但在汇编阶段的内存初始化中好像不是这样的 */
		nr_dram_banks = 8;
		gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
		gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
		gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
		gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
		gd->bd->bi_dram[2].start = PHYS_SDRAM_3;
		gd->bd->bi_dram[2].size = PHYS_SDRAM_3_SIZE;
		gd->bd->bi_dram[3].start = PHYS_SDRAM_4;
		gd->bd->bi_dram[3].size = PHYS_SDRAM_4_SIZE;
		gd->bd->bi_dram[4].start = PHYS_SDRAM_5;
		gd->bd->bi_dram[4].size = PHYS_SDRAM_5_SIZE;
		gd->bd->bi_dram[5].start = PHYS_SDRAM_6;
		gd->bd->bi_dram[5].size = PHYS_SDRAM_6_SIZE;
		gd->bd->bi_dram[6].start = PHYS_SDRAM_7;
		gd->bd->bi_dram[6].size = PHYS_SDRAM_7_SIZE;
		gd->bd->bi_dram[7].start = PHYS_SDRAM_8;
		gd->bd->bi_dram[7].size = PHYS_SDRAM_8_SIZE;
	}

    /* 预留了最后 1MB 空间给 trustzone */
//for trustzone
#ifdef CONFIG_TRUSTZONE
	gd->bd->bi_dram[nr_dram_banks - 1].size -= 0x100000;
#endif

	return 0;
}

int board_late_init (void)
{
    /* 检查启动方式， BOOT_MMCSD(0x03)  /  BOOT_EMMC441(0x07) */
    int ret = check_bootmode();
    if ((ret == BOOT_MMCSD || ret == BOOT_EMMC441 || ret == BOOT_EMMC43 )
            && boot_mode == 0) {
        //printf("board_late_init\n");
        /* 设置启动命令 */
        char boot_cmd[100];
        sprintf(boot_cmd, "movi read kernel 40008000;movi read rootfs 40df0000 100000;bootm 40008000 40df0000");
        setenv("bootcmd", boot_cmd);
    }

    return 0;
}

int checkboard(void)
{
    /* Board:   iTOP-4412 */
	printf("Board:	%s%s\n", CONFIG_DEVICE_STRING,CORE_NUM_STR);
	return (0);
}

ulong virt_to_phy_smdkc210(ulong addr)
{
	if ((0xc0000000 <= addr) && (addr < 0xd0000000))
		return (addr - 0xc0000000 + 0x40000000);

	return addr;
}

#include <movi.h>
int  chk_bootdev(void)//mj for boot device check
{
	char run_cmd[100];
	struct mmc *mmc;
	int boot_dev = 0;
	int cmp_off = 0x10;
	ulong  start_blk, blkcnt;
	
	mmc = find_mmc_device(0);

	if (mmc == NULL)
	{
		printf("There is no eMMC card, Booting device is SD card\n");
		boot_dev = 1;
		return boot_dev;
	}
	start_blk = (24*1024/MOVI_BLKSIZE);
	//start_blk = (24*1024/(1<<9));
	blkcnt = 0x10;
	
	//sprintf(run_cmd,"mmc %s 0 0x%lx 0x%lx 0x%lx",
	//			rw ? "write":"read",
	//			addr, start_blk, blkcnt);

	sprintf(run_cmd,"emmc open 0");
	run_command(run_cmd, 0);
		
	//sprintf(run_cmd,"mmc read 0 0x40008000 0x30 0x20");
	sprintf(run_cmd,"mmc read 0 %lx %lx %lx",CFG_PHY_KERNEL_BASE,start_blk,blkcnt);
	run_command(run_cmd, 0);

	/* switch mmc to normal paritition */
	sprintf(run_cmd,"emmc close 0");
	run_command(run_cmd, 0);

	/* add by cym 20130823 */
	return 0;
	/* end add */

	if (memcmp((void *)(CFG_PHY_UBOOT_BASE+cmp_off),(void *)(CFG_PHY_KERNEL_BASE+cmp_off),blkcnt))
	{
		//printf("mem is different\n");
		boot_dev = 1;
		
	}
	else
	{
		//printf("mem is same\n");
		boot_dev = 0;
	}
	return boot_dev ;


}

/* 检查启动方式,
 * 对于 itop4412 板子，只有两个返回值  0x03  0x07 */
int check_bootmode(void)
{
	int boot_dev =0 ;
	OmPin = INF_REG3_REG;  /* itop4412 只有两个值 BOOT_MMCSD(0x03)  /  BOOT_EMMC441(0x07) */
	
	//printf("\n\nChecking Boot Mode ...");

	if(OmPin == BOOT_ONENAND) {
		printf(" OneNand\n");
	} else if (OmPin == BOOT_NAND) {
		printf(" NAND\n");
	} else if (OmPin == BOOT_MMCSD) {
		printf("\n\nChecking Boot Mode ... SDMMC\n");
	} else if (OmPin == BOOT_EMMC43) { 
		printf(" EMMC4.3\n");
	} else if (OmPin == BOOT_EMMC441) {
		boot_dev = chk_bootdev();
		if(!boot_dev)
			printf("\n\nChecking Boot Mode ... EMMC4.41\n");
		else
		{
			INF_REG3_REG = BOOT_MMCSD;
			OmPin = INF_REG3_REG;
			printf("\n\nChecking Boot Mode ... SDMMC-1\n");
		}
	}
	else
	{
		printf("Undefined booting mode\n");
	}
	return OmPin;
}

