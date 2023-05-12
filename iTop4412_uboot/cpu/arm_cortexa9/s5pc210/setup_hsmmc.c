#include <common.h>
#include <s5pc210.h>
#include <asm/io.h>
#include <mmc.h>
#include <s3c_hsmmc.h>


extern void mshci_fifo_deinit(struct mshci_host *host);
extern struct mmc *find_mmc_device(int dev_num);

void clear_hsmmc_clock_div(void)
{
	/* set sdmmc clk src as mpll */
	u32 tmp;

	struct mmc *mmc = find_mmc_device(0);

	mshci_fifo_deinit(mmc);

	tmp = CLK_SRC_FSYS & ~(0x000fffff);
	CLK_SRC_FSYS = tmp | 0x00066666;


	CLK_DIV_FSYS1 = 0x00080008;
	CLK_DIV_FSYS2 = 0x00080008;
	CLK_DIV_FSYS3 = 0x4;
}

void set_hsmmc_pre_ratio(uint clock)
{
	u32 div;
	u32 tmp;
	u32 clk,sclk_mmc,doutmmc;
	u32 i;
	
	/* MMC2 clock div */
	div = CLK_DIV_FSYS2 & ~(0x0000ff00);
	tmp = CLK_DIV_FSYS2 & (0x0000000f);

	clk = get_MPLL_CLK();
	doutmmc = clk / (tmp + 1);
	
	for(i=0 ; i<=0xff; i++)
	{
		if((doutmmc /(i+1)) <= clock) {
			CLK_DIV_FSYS2 = tmp | i<<8;
			break;
		}
	}
	sclk_mmc = doutmmc/(i+1);
	sddbg("MPLL Clock %dM HZ\n",clk/1000000);
	sddbg("SD DOUTMMC Clock %dM HZ\n",doutmmc/1000000);
	sddbg("SD Host pre-ratio is %x\n",i);
	printf("SD sclk_mmc is %dK HZ\n",sclk_mmc/1000);
	

}

/* mmc 时钟配置函数，使用使用源，选用合适分频进行配置即可 */
/* 按照 4412 手册中时钟单元部分描述， 所有mmc控制器最高时钟频率为 50MHz */
/* sclk_mmc4 记录的是 emmc4 控制器的时钟频率，即 板载 emmc 芯片 */
u32 sclk_mmc4;  //clock source for emmc controller
void setup_hsmmc_clock(void)
{
	u32 tmp;
	u32 clock;
	u32 i;

    /* 时钟源为 MPLL = 800MHz 
     * 按照下面计算，不大于50MHz的分频值，
     * 因为 800 正好可以被 50 整除，所以最终分频之后，时钟就是 50MHz */
	/* MMC2 clock src = SCLKMPLL */
	tmp = CLK_SRC_FSYS & ~(0x00000f00);
	CLK_SRC_FSYS = tmp | 0x00000600;

	/* MMC2 clock div */
	tmp = CLK_DIV_FSYS2 & ~(0x0000000f);
	clock = get_MPLL_CLK()/1000000;
	for(i=0 ; i<=0xf; i++)
	{
		if((clock /(i+1)) <= 50) {
			CLK_DIV_FSYS2 = tmp | i<<0;
			break;
		}
	}
	
	sddbg("[mjdbg] the sd clock ratio is %d,%d\n",i,clock);

    /* MMC4 与 MMC2 类似，只是这里的限制改成了 160MHz，
     * ？？？手册中最高是 50MHz 啊，这里是问什么？？？ */
    /* 这里把 MMC4 的时钟频率记录在了 sclk_mmc4 中，其他文件中有用到 */
	/* MMC4 clock src = SCLKMPLL */
	tmp = CLK_SRC_FSYS & ~(0x000f0000);
	CLK_SRC_FSYS = tmp | 0x00060000;

	/* MMC4 clock div */
	tmp = CLK_DIV_FSYS3 & ~(0x0000ff0f);
	clock = get_MPLL_CLK()/1000000;
	for(i=0 ; i<=0xf; i++)	{
		sclk_mmc4=(clock/(i+1));
		
		if(sclk_mmc4 <= 160) //200
		{
			CLK_DIV_FSYS3 = tmp | (i<<0);
			break;
		}
	}
	emmcdbg("[mjdbg] sclk_mmc4:%d MHZ; mmc_ratio: %d\n",sclk_mmc4,i);
	sclk_mmc4 *= 1000000;
}

/*
 * this will set the GPIO for hsmmc ch0
 * GPG0[0:6] = CLK, CMD, CDn, DAT[0:3]
 */
void setup_hsmmc_cfg_gpio(void)
{
	ulong reg;

    /* MMC2 GPK2 IO初始化,配置为 SD_2_xxxx 模式, GPK2CON */
	writel(0x02222222, 0x11000080);
	writel(0x00003FF0, 0x11000088);
	writel(0x00003FFF, 0x1100008C);
	

    /* MMC4 此处是控制 GPK0_2(SD_4_CDn), 用于给 emmc芯片复位 */
	writel(readl(0x11000048)&~(0xf),0x11000048); //SD_4_CLK/SD_4_CMD pull-down enable
	writel(readl(0x11000040)&~(0xff),0x11000040);//cdn set to be output

	writel(readl(0x11000048)&~(3<<4),0x11000048); //cdn pull-down disable
	writel(readl(0x11000044)&~(1<<2),0x11000044); //cdn output 0 to shutdown the emmc power
	writel(readl(0x11000040)&~(0xf<<8)|(1<<8),0x11000040);//cdn set to be output
	udelay(100*1000);
	writel(readl(0x11000044)|(1<<2),0x11000044); //cdn output 1

	/* MMC4 GPK0 IO初始化,配置为 SD_4_xxxx 模式, GPK0CON */
	writel(0x03333133, 0x11000040);
	writel(0x00003FF0, 0x11000048);
	writel(0x00002AAA, 0x1100004C);

    /* NOTO: 这里的MMC4 在硬件中使用了8bit数据模式，但这里仅初始化了4条数据线
     * 其实这里原本有个编译宏的,用于选择是否使能 8bit 模式，如果使能了，就会
     * 初始化剩下的8条数据线 */
}

void setup_sdhci0_cfg_card(struct sdhci_host *host)
{
}

