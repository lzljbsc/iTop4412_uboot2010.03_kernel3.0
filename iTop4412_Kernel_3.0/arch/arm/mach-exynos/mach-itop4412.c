/* linux/arch/arm/mach-exynos/mach-itop4412.c
 *
 * Copyright (c) 2011 Topeet Electronics Co., Ltd.
 *		http://www.topeetboard.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/clk.h>
#include <linux/lcd.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/i2c.h>

#include <linux/pwm_backlight.h>
#include <linux/input.h>
#include <linux/mmc/host.h>
#include <linux/regulator/machine.h>

#include <linux/regulator/max8649.h>

#include <linux/regulator/fixed.h>

#include <linux/v4l2-mediabus.h>
#include <linux/memblock.h>
#include <linux/delay.h>
#if defined(CONFIG_S5P_MEM_CMA)
#include <linux/cma.h>
#endif
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif

#include <linux/notifier.h>
#include <linux/reboot.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/exynos4.h>
#include <plat/cpu.h>
#include <plat/clock.h>
#include <plat/keypad.h>
#include <plat/devs.h>
#include <plat/fb.h>
#include <plat/fb-s5p.h>
#include <plat/fb-core.h>
#include <plat/regs-fb-v4.h>
#include <plat/backlight.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-adc.h>
#include <plat/adc.h>
#include <plat/iic.h>
#include <plat/pd.h>
#include <plat/sdhci.h>
#include <plat/mshci.h>
#include <plat/ehci.h>
#include <plat/usbgadget.h>
#include <plat/s3c64xx-spi.h>
#if defined(CONFIG_VIDEO_FIMC)
#include <plat/fimc.h>
#endif
#if defined(CONFIG_VIDEO_FIMC_MIPI)
#include <plat/csis.h>
#endif
#include <plat/tvout.h>
#include <plat/media.h>
#include <plat/regs-srom.h>
#include <plat/tv-core.h>

#include <media/exynos_flite.h>
#include <media/exynos_fimc_is.h>
#include <video/platform_lcd.h>
#include <mach/board_rev.h>
#include <mach/map.h>
#include <mach/spi-clocks.h>
#include <mach/exynos-ion.h>
#include <mach/regs-pmu.h>
#include <mach/map.h>
#include <mach/regs-pmu.h>

#if defined(CONFIG_VIDEO_OV5640)
#include <media/soc_camera.h>
#include <media/ov5640.h>
#endif

#ifdef CONFIG_KEYBOARD_GPIO
#include <linux/gpio_keys.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_TSC2007
#include <linux/i2c/tsc2007.h>
#endif

#include <mach/dev.h>
#include <mach/ppmu.h>

#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC) || defined(CONFIG_VIDEO_MFC5X)
#include <plat/s5p-mfc.h>
#endif

#include <plat/fimg2d.h>

#include <mach/dev-sysmmu.h>
#include <plat/sysmmu.h>

#if defined(CONFIG_KERNEL_PANIC_DUMP)		//panic-dump
#include <mach/panic-dump.h>
#endif

#ifdef CONFIG_TC4_ICS
#include <linux/mpu.h>
static struct mpu_platform_data mpu3050_data = {
    .int_config  = 0x10,
    .orientation = {  0,  1,  0, 
        1,  0,  0, 
        0,  0, -1 },
};

/* accel */
static struct ext_slave_platform_data inv_mpu_bma250_data = {
    .bus         = EXT_SLAVE_BUS_SECONDARY,
    .orientation = {  1,  0,  0, 
        0,  -1,  0, 
        0,  0, -1 },
};
#endif

#ifdef CONFIG_USB_NET_DM9620
static void __init dm9620_reset(void)
{
    int err = 0;

    err = gpio_request_one(EXYNOS4_GPC0(1), GPIOF_OUT_INIT_HIGH, "DM9620_RESET");
    if (err)
        pr_err("failed to request GPC0_1 for DM9620 reset control\n");

    s3c_gpio_setpull(EXYNOS4_GPC0(1), S3C_GPIO_PULL_UP);
    gpio_set_value(EXYNOS4_GPC0(1), 0);

    mdelay(1000); //dg change 5 ms to 1000ms for test dm9620 

    gpio_set_value(EXYNOS4_GPC0(1), 1);
    gpio_free(EXYNOS4_GPC0(1));
}
#endif

#include <linux/i2c-gpio.h>	

#if defined (CONFIG_VIDEO_JPEG_V2X) || defined(CONFIG_VIDEO_JPEG)
#include <plat/jpeg.h>
#endif

#ifdef CONFIG_REGULATOR_S5M8767
#include <linux/mfd/s5m87xx/s5m-core.h>
#include <linux/mfd/s5m87xx/s5m-pmic.h>
#endif

#define REG_INFORM4            (S5P_INFORM4)

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDK4X12_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
        S3C2410_UCON_RXILEVEL |	\
        S3C2410_UCON_TXIRQMODE |	\
        S3C2410_UCON_RXIRQMODE |	\
        S3C2410_UCON_RXFIFO_TOI |	\
        S3C2443_UCON_RXERR_IRQEN)

#define SMDK4X12_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMDK4X12_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
        S5PV210_UFCON_TXTRIG4 |	\
        S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg smdk4x12_uartcfgs[] __initdata = {
    [0] = {
        .hwport		= 0,
        .flags		= 0,
        .ucon		= SMDK4X12_UCON_DEFAULT,
        .ulcon		= SMDK4X12_ULCON_DEFAULT,
        .ufcon		= SMDK4X12_UFCON_DEFAULT,
    },
    [1] = {
        .hwport		= 1,
        .flags		= 0,
        .ucon		= SMDK4X12_UCON_DEFAULT,
        .ulcon		= SMDK4X12_ULCON_DEFAULT,
        .ufcon		= SMDK4X12_UFCON_DEFAULT,
    },
    [2] = {
        .hwport		= 2,
        .flags		= 0,
        .ucon		= SMDK4X12_UCON_DEFAULT,
        .ulcon		= SMDK4X12_ULCON_DEFAULT,
        .ufcon		= SMDK4X12_UFCON_DEFAULT,
    },
    [3] = {
        .hwport		= 3,
        .flags		= 0,
        .ucon		= SMDK4X12_UCON_DEFAULT,
        .ulcon		= SMDK4X12_ULCON_DEFAULT,
        .ufcon		= SMDK4X12_UFCON_DEFAULT,
    },
};

#define WRITEBACK_ENABLED

#if defined(CONFIG_VIDEO_FIMC)
/*
 * External camera reset
 * Because the most of cameras take i2c bus signal, so that
 * you have to reset at the boot time for other i2c slave devices.
 * This function also called at fimc_init_camera()
 * Do optimization for cameras on your platform.
 */
#if defined(CONFIG_VIDEO_OV5640)
static int smdk4x12_cam1_reset(int dummy)
{
    return 0;
}
#endif
#endif

#ifdef CONFIG_VIDEO_FIMC
#ifdef CONFIG_VIDEO_OV5640
struct soc_camera_device ov5640_plat = {
    .user_width = 640,
    .user_height = 480,
};
static struct i2c_board_info  ov5640_i2c_info = {
    I2C_BOARD_INFO("ov5640", 0x78>>1),
    .platform_data = &ov5640_plat,
};

static struct s3c_platform_camera ov5640 = {
    .id 	= CAMERA_PAR_A,
    .clk_name	= "sclk_cam0",
    .i2c_busnum = 7,
    .cam_power	= smdk4x12_cam1_reset,

    .type		= CAM_TYPE_ITU,
    .fmt		= ITU_601_YCBCR422_8BIT,
    .order422	= CAM_ORDER422_8BIT_CBYCRY,
    .info		= &ov5640_i2c_info,
    .pixelformat	= V4L2_PIX_FMT_UYVY, //modify by cym V4L2_PIX_FMT_UYVY,
    .srclk_name = "xusbxti",
    .clk_rate	= 24000000,
    .line_length	= 1920,
    .width		= 640,
    .height 	= 480,
    .window 	= {
        .left	= 0,
        .top	= 0,
        .width	= 640,
        .height = 480,
    },

    /* Polarity */
    .inv_pclk	= 0,
    .inv_vsync	= 1,
    .inv_href	= 0,
    .inv_hsync	= 0,
    .reset_camera	= 1,
    .initialized	= 0,
    .layout_rotate = 0 //for shuping, //180, 
};
#endif

/* 2 MIPI Cameras */
#ifdef WRITEBACK_ENABLED
static struct i2c_board_info writeback_i2c_info = {
    I2C_BOARD_INFO("WriteBack", 0x0),
};

static struct s3c_platform_camera writeback = {
    .id		= CAMERA_WB,
    .fmt		= ITU_601_YCBCR422_8BIT,
    .order422	= CAM_ORDER422_8BIT_CBYCRY,
    .i2c_busnum	= 0,
    .info		= &writeback_i2c_info,
    .pixelformat	= V4L2_PIX_FMT_YUV444,
    .line_length	= 800,
    .width		= 480,
    .height		= 800,
    .window		= {
        .left	= 0,
        .top	= 0,
        .width	= 480,
        .height	= 800,
    },

    .initialized	= 0,
};
#endif

/* Interface setting */
static struct s3c_platform_fimc fimc_plat = {
#ifdef WRITEBACK_ENABLED
    .default_cam	= CAMERA_WB,
#endif
    .camera		= {
#ifdef CONFIG_VIDEO_OV5640
        &ov5640,
#endif
#ifdef WRITEBACK_ENABLED
        &writeback,
#endif
    },
    .hw_ver		= 0x51,
};
#endif /* CONFIG_VIDEO_FIMC */


#ifdef CONFIG_S3C64XX_DEV_SPI
static struct s3c64xx_spi_csinfo spi2_csi[] = {
    [0] = {
        .line = EXYNOS4_GPC1(2),
        .set_level = gpio_set_value,
        .fb_delay = 0x2,
    },
};

static struct spi_board_info spi2_board_info[] __initdata = {
#ifdef CONFIG_SPI_RC522
    {
        .modalias = "rc522",
            .platform_data = NULL,
            .max_speed_hz = 10*1000*1000,
            .bus_num = 2,
            .chip_select = 0,
            .mode = SPI_MODE_0,
            .controller_data = &spi2_csi[0],
    }
#endif
};
#endif

static int exynos4_notifier_call(struct notifier_block *this,
        unsigned long code, void *_cmd)
{
    int mode = 0;

    if ((code == SYS_RESTART) && _cmd)
        if (!strcmp((char *)_cmd, "recovery"))
            mode = 0xf;

    __raw_writel(mode, REG_INFORM4);

    return NOTIFY_DONE;
}

static struct notifier_block exynos4_reboot_notifier = {
    .notifier_call = exynos4_notifier_call,
};


#ifdef CONFIG_S3C_DEV_HSMMC2
static struct s3c_sdhci_platdata smdk4x12_hsmmc2_pdata __initdata = {
    .cd_type		= S3C_SDHCI_CD_GPIO,//lisw sd    S3C_SDHCI_CD_INTERNAL,
    .ext_cd_gpio            =EXYNOS4_GPX0(7), //lisw sd
    .ext_cd_gpio_invert     = 1,//lisw sd
    .clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC3
static struct s3c_sdhci_platdata smdk4x12_hsmmc3_pdata __initdata = {
    // SEMCO
#ifdef CONFIG_MTK_COMBO
    .cd_type		= S3C_SDHCI_CD_INTERNAL,
    .clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#endif
};
#endif

#ifdef CONFIG_S5P_DEV_MSHC
static struct s3c_mshci_platdata exynos4_mshc_pdata __initdata = {
    .cd_type		= S3C_MSHCI_CD_PERMANENT,
    .clk_type		= S3C_MSHCI_CLK_DIV_EXTERNAL, //lisw ms
    .has_wp_gpio		= true,
    .wp_gpio		= 0xffffffff,
#if defined(CONFIG_EXYNOS4_MSHC_8BIT) && \
    defined(CONFIG_EXYNOS4_MSHC_DDR)
    .max_width		= 8,
    .host_caps		= MMC_CAP_8_BIT_DATA | MMC_CAP_1_8V_DDR |
        MMC_CAP_UHS_DDR50,
#endif
};
#endif

#ifdef CONFIG_USB_EHCI_S5P
static struct s5p_ehci_platdata smdk4x12_ehci_pdata;

static void __init smdk4x12_ehci_init(void)
{
    struct s5p_ehci_platdata *pdata = &smdk4x12_ehci_pdata;

    s5p_ehci_set_platdata(pdata);
}

// USB3503A, HSIC1 -> USB Host
#define GPIO_HUB_RESET EXYNOS4212_GPM2(4)
#define GPIO_HUB_CONNECT EXYNOS4212_GPM3(3)

void usb_hub_gpio_init(void)
{
    static char flags = 0;

    if(0 == flags){
        gpio_request(GPIO_HUB_RESET, "GPIO_HUB_RESET");
        gpio_direction_output(GPIO_HUB_RESET, 1);
        s3c_gpio_setpull(GPIO_HUB_RESET, S3C_GPIO_PULL_NONE);
        gpio_free(GPIO_HUB_RESET);

        // HUB_CONNECT
        gpio_request(GPIO_HUB_CONNECT, "GPIO_HUB_CONNECT");
        gpio_direction_output(GPIO_HUB_CONNECT, 1);
        s3c_gpio_setpull(GPIO_HUB_CONNECT, S3C_GPIO_PULL_NONE);
        gpio_free(GPIO_HUB_CONNECT);

        flags = 1;
    }

    /* USI 3G Power On*/
#ifdef CONFIG_UNAPLUS
    if(gpio_request(EXYNOS4_GPK1(0), "GPK1_0"))
    {
        printk(KERN_ERR "failed to request GPK1_0 for "
                "USI control\n");
        //return err;
    }

    gpio_direction_output(EXYNOS4_GPK1(0), 0);
    mdelay(50);

    gpio_direction_output(EXYNOS4_GPK1(0), 1);

    s3c_gpio_cfgpin(EXYNOS4_GPK1(0), S3C_GPIO_OUTPUT);
    gpio_free(EXYNOS4_GPK1(0));
    mdelay(50);
#endif
}
#endif

/* USB GADGET */
#ifdef CONFIG_USB_GADGET
static struct s5p_usbgadget_platdata smdk4x12_usbgadget_pdata;

static void __init smdk4x12_usbgadget_init(void)
{
    struct s5p_usbgadget_platdata *pdata = &smdk4x12_usbgadget_pdata;

    s5p_usbgadget_set_platdata(pdata);
}
#endif

static struct platform_device tc4_regulator_consumer = 
{	.name = "tc4-regulator-consumer",	
    .id = -1,
};


/* S5M8767 Regulator */
#ifdef CONFIG_REGULATOR_S5M8767
static int s5m_cfg_irq(void)
{
    /* AP_PMIC_IRQ: EINT15 */
    s3c_gpio_cfgpin(EXYNOS4_GPX1(7), S3C_GPIO_SFN(0xF));
    s3c_gpio_setpull(EXYNOS4_GPX1(7), S3C_GPIO_PULL_UP);
    return 0;
}

//static struct regulator_consumer_supply s5m8767_ldo1_supply[] = {
//	REGULATOR_SUPPLY("vdd_alive", NULL),
//};

static struct regulator_consumer_supply s5m8767_ldo2_supply[] = {
    REGULATOR_SUPPLY("vddq_m12", NULL),
};

//static struct regulator_consumer_supply s5m8767_ldo3_supply[] = {
//	REGULATOR_SUPPLY("vddioap_18", NULL),
//};

//static struct regulator_consumer_supply s5m8767_ldo4_supply[] = {
//	REGULATOR_SUPPLY("vddq_pre", NULL),
//};

static struct regulator_consumer_supply s5m8767_ldo5_supply[] = {
    REGULATOR_SUPPLY("vdd18_2m", NULL),
};

//static struct regulator_consumer_supply s5m8767_ldo6_supply[] = {
//	REGULATOR_SUPPLY("vdd10_mpll", NULL),
//};

//static struct regulator_consumer_supply s5m8767_ldo7_supply[] = {
//	REGULATOR_SUPPLY("vdd10_xpll", NULL),
//};

static struct regulator_consumer_supply s5m8767_ldo8_supply[] = {
    REGULATOR_SUPPLY("vdd10_mipi", NULL),
};

static struct regulator_consumer_supply s5m8767_ldo9_supply[] = {
    REGULATOR_SUPPLY("vdd33_lcd", NULL),
};

static struct regulator_consumer_supply s5m8767_ldo10_supply[] = {
    REGULATOR_SUPPLY("vdd18_mipi", NULL),
};

//static struct regulator_consumer_supply s5m8767_ldo11_supply[] = {
//	REGULATOR_SUPPLY("vdd18_abb1", NULL),
//};

static struct regulator_consumer_supply s5m8767_ldo12_supply[] = {
    REGULATOR_SUPPLY("vdd33_uotg", NULL),
};

//static struct regulator_consumer_supply s5m8767_ldo13_supply[] = {
//	REGULATOR_SUPPLY("vddioperi_18", NULL),
//};

//static struct regulator_consumer_supply s5m8767_ldo14_supply[] = {
//	REGULATOR_SUPPLY("vdd18_abb02", NULL),
//};

static struct regulator_consumer_supply s5m8767_ldo15_supply[] = {
    REGULATOR_SUPPLY("vdd10_ush", NULL),
};

static struct regulator_consumer_supply s5m8767_ldo16_supply[] = {
    REGULATOR_SUPPLY("vdd18_hsic", NULL),
};

//static struct regulator_consumer_supply s5m8767_ldo17_supply[] = {
//	REGULATOR_SUPPLY("vddioap_mmc012_28", NULL),
//};

static struct regulator_consumer_supply s5m8767_ldo18_supply[] = {
    REGULATOR_SUPPLY("vddioperi_28", NULL),
};

static struct regulator_consumer_supply s5m8767_ldo19_supply[] = {
    REGULATOR_SUPPLY("dc33v_tp", NULL),		//cym 20130227
};

static struct regulator_consumer_supply s5m8767_ldo20_supply[] = {
    REGULATOR_SUPPLY("vdd28_cam", NULL),
};
static struct regulator_consumer_supply s5m8767_ldo21_supply[] = {
    REGULATOR_SUPPLY("vdd28_af", NULL),
};

static struct regulator_consumer_supply s5m8767_ldo22_supply[] = {
    REGULATOR_SUPPLY("vdda28_2m", NULL),
};

static struct regulator_consumer_supply s5m8767_ldo23_supply[] = {
    REGULATOR_SUPPLY("vdd_tf", NULL),
};

static struct regulator_consumer_supply s5m8767_ldo24_supply[] = {
    REGULATOR_SUPPLY("vdd33_a31", NULL),
};

static struct regulator_consumer_supply s5m8767_ldo25_supply[] = {
    REGULATOR_SUPPLY("vdd18_cam", NULL),
};

static struct regulator_consumer_supply s5m8767_ldo26_supply[] = {
    REGULATOR_SUPPLY("vdd18_a31", NULL),
};
static struct regulator_consumer_supply s5m8767_ldo27_supply[] = {
    REGULATOR_SUPPLY("gps_1v8", NULL),
};
static struct regulator_consumer_supply s5m8767_ldo28_supply[] = {
    REGULATOR_SUPPLY("dvdd12", NULL),
};


static struct regulator_consumer_supply s5m8767_buck1_consumer =
REGULATOR_SUPPLY("vdd_mif", NULL);

static struct regulator_consumer_supply s5m8767_buck2_consumer =
REGULATOR_SUPPLY("vdd_arm", NULL);

static struct regulator_consumer_supply s5m8767_buck3_consumer =
REGULATOR_SUPPLY("vdd_int", NULL);

static struct regulator_consumer_supply s5m8767_buck4_consumer =
REGULATOR_SUPPLY("vdd_g3d", NULL);

static struct regulator_consumer_supply s5m8767_buck5_consumer =
REGULATOR_SUPPLY("vdd_m12", NULL);

static struct regulator_consumer_supply s5m8767_buck6_consumer =
REGULATOR_SUPPLY("vdd12_5m", NULL);

static struct regulator_consumer_supply s5m8767_buck9_consumer =
REGULATOR_SUPPLY("vddf28_emmc", NULL);



#define REGULATOR_INIT(_ldo, _name, _min_uV, _max_uV, _always_on, _ops_mask,\
        _disabled) \
static struct regulator_init_data s5m8767_##_ldo##_init_data = {		\
    .constraints = {					\
        .name	= _name,				\
        .min_uV = _min_uV,				\
        .max_uV = _max_uV,				\
        .always_on	= _always_on,			\
        .boot_on	= _always_on,			\
        .apply_uV	= 1,				\
        .valid_ops_mask = _ops_mask,			\
        .state_mem	= {				\
            .disabled	= _disabled,		\
            .enabled	= !(_disabled),		\
        }						\
    },							\
    .num_consumer_supplies = ARRAY_SIZE(s5m8767_##_ldo##_supply),	\
    .consumer_supplies = &s5m8767_##_ldo##_supply[0],			\
};

//REGULATOR_INIT(ldo1, "VDD_ALIVE", 1100000, 1100000, 1,
//		REGULATOR_CHANGE_STATUS, 0);

#if defined(CONFIG_CPU_TYPE_SCP_ELITE) || defined(CONFIG_CPU_TYPE_SCP_SUPPER)
REGULATOR_INIT(ldo2, "VDDQ_M12", 1500000, 1500000, 1,
        REGULATOR_CHANGE_STATUS, 1)
#else //pop&pop2G  all bottom borad
REGULATOR_INIT(ldo2, "VDDQ_M12", 1200000, 1200000, 1,
        REGULATOR_CHANGE_STATUS, 1);//sleep controlled by pwren
#endif

//REGULATOR_INIT(ldo3, "VDDIOAP_18", 1800000, 1800000, 1,
//		REGULATOR_CHANGE_STATUS, 0);

//REGULATOR_INIT(ldo4, "VDDQ_PRE", 1800000, 1800000, 1,
//		REGULATOR_CHANGE_STATUS, 1); //sleep controlled by pwren

REGULATOR_INIT(ldo5, "VDD18_2M", 1800000, 1800000, 0,
        REGULATOR_CHANGE_STATUS, 1);

//REGULATOR_INIT(ldo6, "VDD10_MPLL", 1000000, 1000000, 1,
//		REGULATOR_CHANGE_STATUS, 1);//sleep controlled by pwren

//REGULATOR_INIT(ldo7, "VDD10_XPLL", 1000000, 1000000, 1,
//		REGULATOR_CHANGE_STATUS, 1);//sleep controlled by pwren

REGULATOR_INIT(ldo8, "VDD10_MIPI", 1000000, 1000000, 0,
        REGULATOR_CHANGE_STATUS, 1);

#ifdef CONFIG_TOUCHSCREEN_TSC2007
REGULATOR_INIT(ldo9, "VDD33_LCD", 3300000, 3300000, 1,
        REGULATOR_CHANGE_STATUS, 1);
#else
REGULATOR_INIT(ldo9, "VDD33_LCD", 3300000, 3300000, 1,
        REGULATOR_CHANGE_STATUS, 1);
#endif

REGULATOR_INIT(ldo10, "VDD18_MIPI", 1800000, 1800000, 0,
        REGULATOR_CHANGE_STATUS, 1);

//REGULATOR_INIT(ldo11, "VDD18_ABB1", 1800000, 1800000, 1,
//		REGULATOR_CHANGE_STATUS, 0); //???

REGULATOR_INIT(ldo12, "VDD33_UOTG", 3300000, 3300000, 1,
        REGULATOR_CHANGE_STATUS, 0);

//REGULATOR_INIT(ldo13, "VDDIOPERI_18", 1800000, 1800000, 1,
//		REGULATOR_CHANGE_STATUS, 0);//???

//REGULATOR_INIT(ldo14, "VDD18_ABB02", 1800000, 1800000, 1,
//		REGULATOR_CHANGE_STATUS, 0); //???

REGULATOR_INIT(ldo15, "VDD10_USH", 1000000, 1000000, 1,
        REGULATOR_CHANGE_STATUS, 1);

//liang, VDD18_HSIC must be 1.8V, otherwise USB HUB 3503A can't be recognized
REGULATOR_INIT(ldo16, "VDD18_HSIC", 1800000, 1800000, 1,
        REGULATOR_CHANGE_STATUS, 1);

//REGULATOR_INIT(ldo17, "VDDIOAP_MMC012_28", 2800000, 2800000, 1,
//		REGULATOR_CHANGE_STATUS, 0); //???

REGULATOR_INIT(ldo18, "VDDIOPERI_28", 3300000, 3300000, 1,
        REGULATOR_CHANGE_STATUS, 0);//???

REGULATOR_INIT(ldo19, "DC33V_TP", 3300000, 3300000, 0,
        REGULATOR_CHANGE_STATUS, 1); //??

#if defined(CONFIG_VIDEO_OV5640) || defined(CONFIG_VIDEO_TVP5150)
REGULATOR_INIT(ldo20, "VDD28_CAM", 1800000, 1800000, 0,
        REGULATOR_CHANGE_STATUS, 1);
#else
REGULATOR_INIT(ldo20, "VDD28_CAM", 2800000, 2800000, 0,
        REGULATOR_CHANGE_STATUS, 1);
#endif

#ifdef CONFIG_VIDEO_TVP5150
REGULATOR_INIT(ldo21, "VDD28_AF", 1800000, 1800000, 0,
        REGULATOR_CHANGE_STATUS, 1);
#else
REGULATOR_INIT(ldo21, "VDD28_AF", 2800000, 2800000, 0,
        REGULATOR_CHANGE_STATUS, 1);
#endif

REGULATOR_INIT(ldo22, "VDDA28_2M", 2800000, 2800000, 0,
        REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo23, "VDD28_TF", 2800000, 2800000, 1,
        REGULATOR_CHANGE_STATUS, 1);//sleep controlled by pwren

#if 0
REGULATOR_INIT(ldo24, "VDD33_A31", 3300000, 3300000, 1,
        REGULATOR_CHANGE_STATUS, 0);
#else
#if defined(CONFIG_MTK_COMBO_COMM) || defined(CONFIG_MTK_COMBO_COMM_MODULE)
REGULATOR_INIT(ldo24, "VDD33_A31", 3300000, 3300000, 1,
        REGULATOR_CHANGE_STATUS, 0);
#else
REGULATOR_INIT(ldo24, "VDD33_A31", 3300000, 3300000, 1,
        REGULATOR_CHANGE_STATUS, 0);
#endif
#endif

REGULATOR_INIT(ldo25, "VDD18_CAM", 1800000, 1800000, 0,
        REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo26, "VDD18_A31", 1800000, 1800000, 1,
        REGULATOR_CHANGE_STATUS, 0);

REGULATOR_INIT(ldo27, "GPS_1V8", 1800000, 1800000, 1,
        REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo28, "DVDD12", 1200000, 1200000, 0,
        REGULATOR_CHANGE_STATUS, 1);


static struct regulator_init_data s5m8767_buck1_data = {
    .constraints	= {
        .name		= "vdd_mif range",
        .min_uV		= 900000,
        .max_uV		= 1100000,
        .valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
            REGULATOR_CHANGE_STATUS,
        .state_mem	= {
            .disabled	= 1,
        },
    },
    .num_consumer_supplies	= 1,
    .consumer_supplies	= &s5m8767_buck1_consumer,
};

static struct regulator_init_data s5m8767_buck2_data = {
    .constraints	= {
        .name		= "vdd_arm range",
        .min_uV		=  850000,
        .max_uV		= 1450000,
        .valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
            REGULATOR_CHANGE_STATUS,
        .state_mem	= {
            .disabled	= 1,
        },
    },
    .num_consumer_supplies	= 1,
    .consumer_supplies	= &s5m8767_buck2_consumer,
};

static struct regulator_init_data s5m8767_buck3_data = {
    .constraints	= {
        .name		= "vdd_int range",
        .min_uV		=  875000,
        .max_uV		= 1200000,
        .apply_uV	= 1,
        .valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
            REGULATOR_CHANGE_STATUS,
        .state_mem	= {
            //.uV		= 1100000,
            .mode		= REGULATOR_MODE_NORMAL,
            .disabled	= 1,
        },
    },
    .num_consumer_supplies	= 1,
    .consumer_supplies	= &s5m8767_buck3_consumer,
};

static struct regulator_init_data s5m8767_buck4_data = {
    .constraints	= {
        .name		= "vdd_g3d range",
        .min_uV		= 750000,
        .max_uV		= 1500000,
        .valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
            REGULATOR_CHANGE_STATUS,
        .state_mem	= {
            .disabled	= 1,
        },
    },
    .num_consumer_supplies = 1,
    .consumer_supplies = &s5m8767_buck4_consumer,
};

static struct regulator_init_data s5m8767_buck5_data = {
    .constraints	= {
        .name		= "vdd_m12 range",
        .min_uV		= 750000,
        .max_uV		= 1500000,
        .apply_uV	= 1,
        .boot_on	= 1,
        .valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
            REGULATOR_CHANGE_STATUS,
        .state_mem	= {
            .enabled	= 1,
        },
    },
    .num_consumer_supplies = 1,
    .consumer_supplies = &s5m8767_buck5_consumer,
};

static struct regulator_init_data s5m8767_buck6_data = {
    .constraints	= {
        .name		= "vdd12_5m range",
        .min_uV		= 750000,
        .max_uV		= 1500000,
        .boot_on	= 0,
        .apply_uV	= 1,
        .valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
            REGULATOR_CHANGE_STATUS,
        .state_mem	= {
            .disabled	= 1,
        },
    },
    .num_consumer_supplies = 1,
    .consumer_supplies = &s5m8767_buck6_consumer,
};

static struct regulator_init_data s5m8767_buck9_data = {
    .constraints	= {
        .name		= "vddf28_emmc range",
        .min_uV		= 750000,
        .max_uV		= 3000000,
        .boot_on	= 1,
        .apply_uV	= 1,
        .valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
            REGULATOR_CHANGE_STATUS,
        .state_mem	= {
            .disabled	= 1,
        },
    },
    .num_consumer_supplies = 1,
    .consumer_supplies = &s5m8767_buck9_consumer,
};


static struct s5m_regulator_data pegasus_regulators[] = {
    { S5M8767_BUCK1, &s5m8767_buck1_data },
    { S5M8767_BUCK2, &s5m8767_buck2_data },
    { S5M8767_BUCK3, &s5m8767_buck3_data },
    { S5M8767_BUCK4, &s5m8767_buck4_data },
    { S5M8767_BUCK5, &s5m8767_buck5_data },
    { S5M8767_BUCK6, &s5m8767_buck6_data },
    { S5M8767_BUCK9, &s5m8767_buck9_data },

    //{ S5M8767_LDO1, &s5m8767_ldo1_init_data },
    { S5M8767_LDO2, &s5m8767_ldo2_init_data },
    //{ S5M8767_LDO3, &s5m8767_ldo3_init_data },
    //{ S5M8767_LDO4, &s5m8767_ldo4_init_data },
    { S5M8767_LDO5, &s5m8767_ldo5_init_data },
    //{ S5M8767_LDO6, &s5m8767_ldo6_init_data },
    //{ S5M8767_LDO7, &s5m8767_ldo7_init_data },
    { S5M8767_LDO8, &s5m8767_ldo8_init_data },
    { S5M8767_LDO9, &s5m8767_ldo9_init_data },
    { S5M8767_LDO10, &s5m8767_ldo10_init_data },

    //{ S5M8767_LDO11, &s5m8767_ldo11_init_data },
    { S5M8767_LDO12, &s5m8767_ldo12_init_data },
    //{ S5M8767_LDO13, &s5m8767_ldo13_init_data },
    //{ S5M8767_LDO14, &s5m8767_ldo14_init_data },
    { S5M8767_LDO15, &s5m8767_ldo15_init_data },
    { S5M8767_LDO16, &s5m8767_ldo16_init_data },
    //{ S5M8767_LDO17, &s5m8767_ldo17_init_data },
    { S5M8767_LDO18, &s5m8767_ldo18_init_data },
    { S5M8767_LDO19, &s5m8767_ldo19_init_data },
    { S5M8767_LDO20, &s5m8767_ldo20_init_data },

    { S5M8767_LDO21, &s5m8767_ldo21_init_data },
    { S5M8767_LDO22, &s5m8767_ldo22_init_data },
    { S5M8767_LDO23, &s5m8767_ldo23_init_data },
    { S5M8767_LDO24, &s5m8767_ldo24_init_data },
    { S5M8767_LDO25, &s5m8767_ldo25_init_data },
    { S5M8767_LDO26, &s5m8767_ldo26_init_data },
    { S5M8767_LDO27, &s5m8767_ldo27_init_data },
    { S5M8767_LDO28, &s5m8767_ldo28_init_data },
};

static struct s5m_platform_data exynos4_s5m8767_pdata = {
    .device_type		= S5M8767X,
    .irq_base		= IRQ_BOARD_START,
    .num_regulators		= ARRAY_SIZE(pegasus_regulators),
    .regulators		= pegasus_regulators,
    .cfg_pmic_irq		= s5m_cfg_irq,
    .buck_ramp_delay        = 10,
};
#endif
/* End of S5M8767 */


/* i2c devs mapping: 
 * i2c0 : HDMI
 * i2c1 : max8997: PMIC & RTC & motor
 * i2c2 : not used
 * i2c3 : touch
 * i2c4 : max8997 fuel gauge & wm8960
 * i2c5 : sensor: MPU3050
 * i2c6 : camera & HSIC
 * i2c7 : light sensor
 */
static struct i2c_board_info i2c_devs0[] __initdata = {
#ifdef CONFIG_VIDEO_TVOUT
    {
        I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)),    
    },
#endif
};

static struct i2c_board_info i2c_devs1[] __initdata = {
#ifdef CONFIG_REGULATOR_S5M8767
    {
        I2C_BOARD_INFO("s5m87xx", 0xCC >> 1),
        .platform_data = &exynos4_s5m8767_pdata,
        .irq		= IRQ_EINT(15),
    },
#endif
};

static struct i2c_board_info i2c_devs2[] __initdata = {

};

static struct i2c_board_info i2c_devs6[] __initdata = {

};

#ifdef CONFIG_TOUCHSCREEN_FT5X0X
#include <plat/ft5x0x_touch.h>
static struct ft5x0x_i2c_platform_data ft5x0x_pdata = {
    .gpio_irq               = EXYNOS4_GPX0(4),
    .irq_cfg                = S3C_GPIO_SFN(0xf),
    .screen_max_x   = 768,
    .screen_max_y   = 1024,
    .pressure_max   = 255,
};
#endif

void init_lcd_type(void)
{
    if (gpio_request(EXYNOS4_GPC0(3), "GPC0_3"))
        printk(KERN_WARNING "GPC0_3 Port request error!!!\n");
    else {
        s3c_gpio_setpull(EXYNOS4_GPC0(3), S3C_GPIO_PULL_NONE);
        s3c_gpio_cfgpin(EXYNOS4_GPC0(3), S3C_GPIO_SFN(0));
        gpio_direction_input(EXYNOS4_GPC0(3));
        //gpio_free(EXYNOS4_GPC0(3));
    }

    if (gpio_request(EXYNOS4_GPX0(6), "GPX0_6"))
        printk(KERN_WARNING "GPX0_6 Port request error!!!\n");
    else {
        s3c_gpio_setpull(EXYNOS4_GPX0(6), S3C_GPIO_PULL_NONE);
        s3c_gpio_cfgpin(EXYNOS4_GPX0(6), S3C_GPIO_SFN(0));
        gpio_direction_input(EXYNOS4_GPX0(6));
        //gpio_free(EXYNOS4_GPX0(6));
    }
}

int get_lcd_type(void)
{
    int value1, value2, type = 0;

    value1 = gpio_get_value(EXYNOS4_GPC0(3));
    value2 = gpio_get_value(EXYNOS4_GPX0(6));

    type = (value1<<1)|value2;

    printk("value1 = %d, value2 = %d, type = 0x%x\n", value1, value2, type);

    return type;
}
EXPORT_SYMBOL(get_lcd_type);

void setup_ft5x_width_height(void)
{
    int type = get_lcd_type();

    if(0x00 == type)	//9.7
    {
#if defined(CONFIG_TOUCHSCREEN_FT5X0X)
        ft5x0x_pdata.screen_max_x = 768;
        ft5x0x_pdata.screen_max_y = 1024;
#endif
        ;
    }
    else if(0x01 == type)	//7.0
    {
#if defined(CONFIG_TOUCHSCREEN_FT5X0X)
        ft5x0x_pdata.screen_max_x = 1280;//1280;
        ft5x0x_pdata.screen_max_y = 800;//800;
#endif
        ;
    }
    else if(0x02 == type)	//4.3
    {
        ;
    }
}

static struct i2c_board_info i2c_devs3[] __initdata = {
    /* support for FT5X0X TouchScreen */
#if defined(CONFIG_TOUCHSCREEN_FT5X0X)
    {
        I2C_BOARD_INFO("ft5x0x_ts", 0x70>>1),
        .irq = IRQ_EINT(4),
        .platform_data = &ft5x0x_pdata,
    },
#endif
};

#ifdef CONFIG_SND_SOC_WM8960
#include <sound/wm8960.h>
static struct wm8960_data wm8960_pdata = {
    .capless		= 0,
    .dres			= WM8960_DRES_400R,
};
#endif

/* I2C4 */
static struct i2c_board_info i2c_devs4[] __initdata = {
#ifdef CONFIG_SND_SOC_WM8960
    {
        I2C_BOARD_INFO("wm8960", 0x1a),
        .platform_data	= &wm8960_pdata,
    },
#endif
};

/* I2C5 */
static struct i2c_board_info i2c_devs5[] __initdata = {
#ifdef CONFIG_TC4_ICS
    {
        I2C_BOARD_INFO(MPU_NAME, 0x68),
        .irq = IRQ_EINT(27),
        .platform_data = &mpu3050_data,
    },
    {
        I2C_BOARD_INFO("bma250", (0x30>>1)),
        //.irq = IRQ_EINT(24),// 25?
        .platform_data = &inv_mpu_bma250_data,
    },
#endif
};

#if  defined(CONFIG_CPU_TYPE_SCP_ELITE)
/* for TSC2007 TouchScreen */
#ifdef CONFIG_TOUCHSCREEN_TSC2007
#define GPIO_TSC_PORT EXYNOS4_GPX0(0)
static int ts_get_pendown_state(void)
{
    int val;

    val = gpio_get_value(GPIO_TSC_PORT);

    return !val;
}

static int ts_init(void)
{
    int err;

    err = gpio_request_one(EXYNOS4_GPX0(0), GPIOF_IN, "TSC2007_IRQ");
    if (err) {
        printk(KERN_ERR "failed to request TSC2007_IRQ pin\n");
        return -1;
    }

    s3c_gpio_cfgpin(EXYNOS4_GPX0(0), S3C_GPIO_SFN(0xF));
    s3c_gpio_setpull(EXYNOS4_GPX0(0), S3C_GPIO_PULL_NONE);
    gpio_free(EXYNOS4_GPX0(0));

    return 0;
}

static struct tsc2007_platform_data tsc2007_info = {
    .model			    = 2007,
    .x_plate_ohms		= 180,
    .get_pendown_state	= ts_get_pendown_state,
    .init_platform_hw	= ts_init,
};
#endif
#endif

/* I2C7 */
static struct i2c_board_info i2c_devs7[] __initdata = {
#if defined(CONFIG_CPU_TYPE_SCP_ELITE)
/* for TSC2007 TouchScreen */
#ifdef CONFIG_TOUCHSCREEN_TSC2007
    {
        I2C_BOARD_INFO("tsc2007", 0x48),
        .type	    	= "tsc2007",
        .platform_data	= &tsc2007_info,
        .irq = IRQ_EINT(0),
    }
#endif
#endif
};

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
    .name		= "pmem",
    .no_allocator	= 1,
    .cached		= 0,
    .start		= 0,
    .size		= 0
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
    .name		= "pmem_gpu1",
    .no_allocator	= 1,
    .cached		= 0,
    .start		= 0,
    .size		= 0,
};

static struct platform_device pmem_device = {
    .name	= "android_pmem",
    .id	= 0,
    .dev	= {
        .platform_data = &pmem_pdata
    },
};

static struct platform_device pmem_gpu1_device = {
    .name	= "android_pmem",
    .id	= 1,
    .dev	= {
        .platform_data = &pmem_gpu1_pdata
    },
};

static void __init android_pmem_set_platdata(void)
{
#if defined(CONFIG_S5P_MEM_CMA)
    pmem_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM * SZ_1K;
    pmem_gpu1_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1 * SZ_1K;
#endif
}
#endif

/* s5p-pmic interface */
static struct resource s5p_pmic_resource[] = {

};

struct platform_device s5p_device_pmic = {
    .name             = "s5p-pmic",
    .id               = -1,
    .num_resources    = ARRAY_SIZE(s5p_pmic_resource),
    .resource         = s5p_pmic_resource,
};
EXPORT_SYMBOL(s5p_device_pmic);

#ifdef CONFIG_SWITCH_GPIO
#include <linux/switch.h>
static struct gpio_switch_platform_data headset_switch_data = {
    .name = "h2w",
    .gpio = EXYNOS4_GPX2(2), // "GPX2"
};

static struct resource switch_gpio_resource[] = {
    [0] = {
        .start  = IRQ_EINT(18), // WAKEUP_INT2[2]
        .end    = IRQ_EINT(18),
        .flags  = IORESOURCE_IRQ,
    },
};

static struct platform_device headset_switch_device = {
    .name             = "switch-gpio",
    .dev = {
        .platform_data    = &headset_switch_data,
    },
    .num_resources  = ARRAY_SIZE(switch_gpio_resource),
    .resource = switch_gpio_resource,
};
#endif

static void __init smdk4x12_gpio_power_init(void)
{
    /* for TSC2007 TouchScreen */
#ifndef CONFIG_TOUCHSCREEN_TSC2007
    int err = 0;
    err = gpio_request_one(EXYNOS4_GPX0(0), 0, "GPX0");
    if (err) {
        printk(KERN_ERR "failed to request GPX0 for "
                "suspend/resume control\n");
        return;
    }
    s3c_gpio_setpull(EXYNOS4_GPX0(0), S3C_GPIO_PULL_NONE);
    gpio_free(EXYNOS4_GPX0(0));
#endif
}

#ifdef CONFIG_KEYBOARD_GPIO
#if defined(CONFIG_CPU_TYPE_SCP_ELITE)
static struct gpio_keys_button gpio_buttons[] = {
    {
        .gpio		= EXYNOS4_GPX1(1),
        //.code		= 10,
        .code		= KEY_HOMEPAGE,
        .desc		= "BUTTON1",
        .active_low	= 1,
        .wakeup		= 0,
    },
    {
        .gpio		= EXYNOS4_GPX1(2),
        //.code		= 24,
        .code		= KEY_BACK,
        .desc		= "BUTTON2",
        .active_low	= 1,
        .wakeup		= 0,
    },
    {
        .gpio		= EXYNOS4_GPX3(3),
        //.code		= 38,
        .code		= KEY_POWER,
        .desc		= "BUTTON3",
        .active_low	= 1,
        .wakeup		= 0,
    },
    {
        .gpio           = EXYNOS4_GPX2(0),
        //.code         = 38,
        .code           = KEY_VOLUMEDOWN,
        .desc           = "BUTTON4",
        .active_low     = 1,
        .wakeup         = 0,
    },
    {
        .gpio           = EXYNOS4_GPX2(1),
        //.code         = 38,
        .code           = KEY_VOLUMEUP,
        .desc           = "BUTTON5",
        .active_low     = 1,
        .wakeup         = 0,
    }
};
#endif

static struct gpio_keys_platform_data gpio_button_data = {
    .buttons	= gpio_buttons,
    .nbuttons	= ARRAY_SIZE(gpio_buttons),
};

static struct platform_device gpio_button_device = {
    .name	= "gpio-keys",
    .id		= -1,
    .num_resources	= 0,
    .dev	= {
        .platform_data	= &gpio_button_data,
    }
};
#endif

#ifdef CONFIG_SAMSUNG_DEV_KEYPAD
#if defined(CONFIG_CPU_TYPE_SCP_ELITE)
static uint32_t smdk4x12_keymap[] __initdata = {
    /* KEY(row, col, keycode) */
    KEY(8, 0, KEY_1), KEY(8, 3, KEY_2), KEY(8, 5, KEY_3), KEY(8, 6, KEY_4),
    KEY(6, 0, KEY_5), KEY(6, 3, KEY_6), KEY(6, 5, KEY_7), KEY(6, 6, KEY_8),
    KEY(7, 0, KEY_9), KEY(7, 3, KEY_0), KEY(7, 5, KEY_A), KEY(7, 6, KEY_B),
    KEY(13, 0, KEY_C), KEY(13, 3, KEY_D), KEY(13, 5, KEY_E), KEY(13, 6, KEY_F)

};

static struct matrix_keymap_data smdk4x12_keymap_data __initdata = {
    .keymap		= smdk4x12_keymap,
    .keymap_size	= ARRAY_SIZE(smdk4x12_keymap),
};

static struct samsung_keypad_platdata smdk4x12_keypad_data __initdata = {
    .keymap_data	= &smdk4x12_keymap_data,
    .rows		= 14,
    .cols		= 7,

};
#endif
#endif

#ifdef CONFIG_VIDEO_FIMG2D
static struct fimg2d_platdata fimg2d_data __initdata = {
    .hw_ver = 0x41,
    .parent_clkname = "mout_g2d0",
    .clkname = "sclk_fimg2d",
    .gate_clkname = "fimg2d",
    .clkrate = 267 * 1000000,	/* 266 Mhz */
};
#endif

#ifdef CONFIG_BUSFREQ_OPP
/* BUSFREQ to control memory/bus*/
static struct device_domain busfreq;
#endif

static struct platform_device exynos4_busfreq = {
    .id = -1,
    .name = "exynos-busfreq",
};

// SEMCO
/* The sdhci_s3c_sdio_card_detect function is used for detecting
   the WiFi/BT module when the menu for enabling the WiFi is
   selected.
   The semco_a31_detection function is called by ar6000's probe function.

   The call sequence is

   ar6000_pm_probe() -> plat_setup_power_for_onoff() -> detect_semco_wlan_for_onoff()
   -> setup_semco_wlan_power_onoff() -> semco_a31_detection()

   The mmc_semco_a31_sdio_remove function is used for removing the mmc driver
   when the menu for disabling the WiFi is selected.
   The semco_a31_removal function is called by ar6000's remove function.

   The call sequence is

   ar6000_pm_remove() -> plat_setup_power_for_onoff() -> detect_semco_wlan_for_onoff()
   -> setup_semco_wlan_power_onoff() -> semco_a31_removal()

   The setup_semco_wlan_power function is only used for sleep/wakeup. It controls only 
   the power of A31 module only(Do not card detection/removal function)
   */

extern void sdhci_s3c_sdio_card_detect(struct platform_device *pdev);
void semco_a31_detection(void)
{
    sdhci_s3c_sdio_card_detect(&s3c_device_hsmmc3);
}
EXPORT_SYMBOL(semco_a31_detection);


extern void mmc_semco_a31_sdio_remove(void);
void semco_a31_removal(void)
{
    mmc_semco_a31_sdio_remove();
}
EXPORT_SYMBOL(semco_a31_removal);

static struct platform_device s3c_wlan_ar6000_pm_device = {
    .name           = "wlan_ar6000_pm_dev",
    .id             = 1,
    .num_resources  = 0,
    .resource       = NULL,
};

static struct platform_device bt_sysfs = {
    .name = "bt-sysfs",
    .id = -1,
};
// END SEMCO

struct platform_device s3c_device_gps = {
    .name   = "si_gps",
    .id     = -1,
};

#ifdef CONFIG_MAX485_CTL
struct platform_device s3c_device_max485_ctl = {
    .name   = "max485_ctl",
    .id     = -1,
};
#endif

#ifdef CONFIG_LEDS_CTL
struct platform_device s3c_device_leds_ctl = {
    .name   = "leds",
    .id     = -1,
};
#endif

#ifdef CONFIG_BUZZER_CTL
struct platform_device s3c_device_buzzer_ctl = {
    .name   = "buzzer_ctl",
    .id     = -1,
};
#endif

#ifdef CONFIG_ADC_CTL
struct platform_device s3c_device_adc_ctl = {
    .name   = "adc_ctl",
    .id     = -1,
};
#endif

#if  defined(CONFIG_CPU_TYPE_SCP_ELITE)
#ifdef CONFIG_RELAY_CTL
struct platform_device s3c_device_relay_ctl = {
    .name   = "relay_ctl",
    .id     = -1,
};
#endif
#endif

static  struct  i2c_gpio_platform_data  i2c0_platdata = {
    .sda_pin                = EXYNOS4_GPD1(0),
    .scl_pin                = EXYNOS4_GPD1(1),
    .udelay                 = 1 ,  
    .sda_is_open_drain      = 0,
    .scl_is_open_drain      = 0,
    .scl_is_output_only     = 0,
    //      .scl_is_output_only     = 1,
};

static struct platform_device s3c_device_i2c0_gpio = {
    .name                   = "i2c-gpio",
    .id                     = 0,
    .dev.platform_data      = &i2c0_platdata,
};

static struct platform_device *smdk4412_devices[] __initdata = {
    &s3c_device_adc,
};

static struct platform_device *smdk4x12_devices[] __initdata = {
#ifdef CONFIG_ANDROID_PMEM
    &pmem_device,
    &pmem_gpu1_device,
#endif
    /* Samsung Power Domain */
    &exynos4_device_pd[PD_MFC],
    &exynos4_device_pd[PD_G3D],
    &exynos4_device_pd[PD_LCD0],
    &exynos4_device_pd[PD_CAM],
    &exynos4_device_pd[PD_TV],
    &exynos4_device_pd[PD_GPS],
    &exynos4_device_pd[PD_GPS_ALIVE],
    /* legacy fimd */
#ifdef CONFIG_FB_S5P
    &s3c_device_fb,
#endif
    &s3c_device_wdt,
    &s3c_device_rtc,
    //&s3c_device_i2c0,
    &s3c_device_i2c0_gpio,
    &s3c_device_i2c1,
    //&s3c_device_i2c2,
    &s3c_device_i2c3,
    &s3c_device_i2c4,
    &s3c_device_i2c5,
    &s3c_device_i2c7,

#if !defined(CONFIG_REGULATOR_MAX8997)	
    &s5p_device_pmic,
#endif	

    &tc4_regulator_consumer,

#ifdef CONFIG_USB_EHCI_S5P
    &s5p_device_ehci,
#endif
#ifdef CONFIG_USB_GADGET
    &s3c_device_usbgadget,
#endif
    // SEMCO
    &s3c_wlan_ar6000_pm_device,
    &bt_sysfs,
    // END SEMCO

#ifdef CONFIG_S5P_DEV_MSHC
    &s3c_device_mshci,//lisw sd mshci should be probe before hsmmc
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
    &s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
    &s3c_device_hsmmc3,
#endif
#ifdef CONFIG_SND_SAMSUNG_I2S
    &exynos_device_i2s0,
#endif
#if defined(CONFIG_SND_SAMSUNG_RP) || defined(CONFIG_SND_SAMSUNG_ALP)
    &exynos_device_srp,
#endif
#ifdef CONFIG_VIDEO_TVOUT
    &s5p_device_tvout,
    &s5p_device_cec,
    &s5p_device_hpd,
#endif
#if defined(CONFIG_VIDEO_FIMC)
    &s3c_device_fimc0,
    &s3c_device_fimc1,
    &s3c_device_fimc2,
    &s3c_device_fimc3,
#endif
#if defined(CONFIG_VIDEO_FIMC_MIPI)
    &s3c_device_csis0,
    &s3c_device_csis1,
#endif
#if defined(CONFIG_VIDEO_MFC5X) || defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
    &s5p_device_mfc,
#endif
#ifdef CONFIG_S5P_SYSTEM_MMU
    &SYSMMU_PLATDEV(g2d_acp),
    &SYSMMU_PLATDEV(fimc0),
    &SYSMMU_PLATDEV(fimc1),
    &SYSMMU_PLATDEV(fimc2),
    &SYSMMU_PLATDEV(fimc3),
    &SYSMMU_PLATDEV(jpeg),
    &SYSMMU_PLATDEV(mfc_l),
    &SYSMMU_PLATDEV(mfc_r),
    &SYSMMU_PLATDEV(tv),
#endif /* CONFIG_S5P_SYSTEM_MMU */

#ifdef CONFIG_ION_EXYNOS
    &exynos_device_ion,
#endif
#ifdef CONFIG_VIDEO_FIMG2D
    &s5p_device_fimg2d,
#endif
#if	defined(CONFIG_VIDEO_JPEG_V2X) || defined(CONFIG_VIDEO_JPEG)
    &s5p_device_jpeg,
#endif
    &samsung_asoc_dma,
    &samsung_asoc_idma,
    &samsung_device_keypad,

#ifdef CONFIG_KEYBOARD_GPIO
    &gpio_button_device,
#endif

#ifdef CONFIG_S3C64XX_DEV_SPI
    &exynos_device_spi2,
#endif
    &exynos4_busfreq,
#ifdef CONFIG_SWITCH_GPIO
    &headset_switch_device,
#endif
    &s3c_device_gps,
#ifdef CONFIG_MAX485_CTL
    &s3c_device_max485_ctl ,
#endif
#ifdef CONFIG_LEDS_CTL
    &s3c_device_leds_ctl,
#endif
#ifdef CONFIG_BUZZER_CTL
    &s3c_device_buzzer_ctl,
#endif
#ifdef CONFIG_ADC_CTL
    &s3c_device_adc_ctl,
#endif

#if  defined(CONFIG_CPU_TYPE_SCP_ELITE)
#ifdef CONFIG_RELAY_CTL
    &s3c_device_relay_ctl,
#endif
#endif
};

#if defined(CONFIG_VIDEO_TVOUT)
static struct s5p_platform_hpd hdmi_hpd_data __initdata = {

};
static struct s5p_platform_cec hdmi_cec_data __initdata = {

};
#endif

#if defined(CONFIG_S5P_MEM_CMA)
static void __init exynos4_cma_region_reserve(
        struct cma_region *regions_normal,
        struct cma_region *regions_secure)
{
    struct cma_region *reg;
    phys_addr_t paddr_last = 0xFFFFFFFF;

    for (reg = regions_normal; reg->size != 0; reg++) {
        phys_addr_t paddr;

        if (!IS_ALIGNED(reg->size, PAGE_SIZE)) {
            pr_err("S5P/CMA: size of '%s' is NOT page-aligned\n",
                    reg->name);
            reg->size = PAGE_ALIGN(reg->size);
        }

        if (reg->reserved) {
            pr_err("S5P/CMA: '%s' alread reserved\n", reg->name);
            continue;
        }

        if (reg->alignment) {
            if ((reg->alignment & ~PAGE_MASK) ||
                    (reg->alignment & ~reg->alignment)) {
                pr_err("S5P/CMA: Failed to reserve '%s': "
                        "incorrect alignment 0x%08x.\n",
                        reg->name, reg->alignment);
                continue;
            }
        } else {
            reg->alignment = PAGE_SIZE;
        }

        if (reg->start) {
            if (!memblock_is_region_reserved(reg->start, reg->size)
                    && (memblock_reserve(reg->start, reg->size) == 0))
                reg->reserved = 1;
            else
                pr_err("S5P/CMA: Failed to reserve '%s'\n",
                        reg->name);

            continue;
        }

        paddr = memblock_find_in_range(0, MEMBLOCK_ALLOC_ACCESSIBLE,
                reg->size, reg->alignment);
        if (paddr != MEMBLOCK_ERROR) {
            if (memblock_reserve(paddr, reg->size)) {
                pr_err("S5P/CMA: Failed to reserve '%s'\n",
                        reg->name);
                continue;
            }

            reg->start = paddr;
            reg->reserved = 1;
        } else {
            pr_err("S5P/CMA: No free space in memory for '%s'\n",
                    reg->name);
        }

        if (cma_early_region_register(reg)) {
            pr_err("S5P/CMA: Failed to register '%s'\n",
                    reg->name);
            memblock_free(reg->start, reg->size);
        } else {
            paddr_last = min(paddr, paddr_last);
        }
    }

    if (regions_secure && regions_secure->size) {
        size_t size_secure = 0;
        size_t align_secure, size_region2, aug_size, order_region2;

        for (reg = regions_secure; reg->size != 0; reg++)
            size_secure += reg->size;

        reg--;

        /* Entire secure regions will be merged into 2
         * consecutive regions. */
        align_secure = 1 <<
            (get_order((size_secure + 1) / 2) + PAGE_SHIFT);
        /* Calculation of a subregion size */
        size_region2 = size_secure - align_secure;
        order_region2 = get_order(size_region2) + PAGE_SHIFT;
        if (order_region2 < 20)
            order_region2 = 20; /* 1MB */
        order_region2 -= 3; /* divide by 8 */
        size_region2 = ALIGN(size_region2, 1 << order_region2);

        aug_size = align_secure + size_region2 - size_secure;
        if (aug_size > 0)
            reg->size += aug_size;

        size_secure = ALIGN(size_secure, align_secure);

        if (paddr_last >= memblock.current_limit) {
            paddr_last = memblock_find_in_range(0,
                    MEMBLOCK_ALLOC_ACCESSIBLE,
                    size_secure, reg->alignment);
        } else {
            paddr_last -= size_secure;
            paddr_last = round_down(paddr_last, align_secure);
        }

        if (paddr_last) {
            while (memblock_reserve(paddr_last, size_secure))
                paddr_last -= align_secure;

            do {
                reg->start = paddr_last;
                reg->reserved = 1;
                paddr_last += reg->size;

                if (cma_early_region_register(reg)) {
                    memblock_free(reg->start, reg->size);
                    pr_err("S5P/CMA: "
                            "Failed to register secure region "
                            "'%s'\n", reg->name);
                } else {
                    size_secure -= reg->size;
                }
            } while (reg-- != regions_secure);

            if (size_secure > 0)
                memblock_free(paddr_last, size_secure);
        } else {
            pr_err("S5P/CMA: Failed to reserve secure regions\n");
        }
    }
}

static void __init exynos4_reserve_mem(void)
{
    static struct cma_region regions[] = {
#ifdef CONFIG_ANDROID_PMEM_MEMSIZE_PMEM
        {
            .name = "pmem",
            .size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM * SZ_1K,
            .start = 0,
        },
#endif
#ifdef CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1
        {
            .name = "pmem_gpu1",
            .size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1 * SZ_1K,
            .start = 0,
        },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG
        {
            .name = "jpeg",
            .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG * SZ_1K,
            .start = 0
        },
#endif
#ifdef CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP
        {
            .name = "srp",
            .size = CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP * SZ_1K,
            .start = 0,
        },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D
        {
            .name = "fimg2d",
            .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D * SZ_1K,
            .start = 0
        },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD
        {
            .name = "fimd",
            .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD * SZ_1K,
            .start = 0
        },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0
        {
            .name = "fimc0",
            .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0 * SZ_1K,
            .start = 0
        },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2
        {
            .name = "fimc2",
            .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2 * SZ_1K,
            .start = 0
        },
#endif
#if !defined(CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION) && \
        defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3)
        {
            .name = "fimc3",
            .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3 * SZ_1K,
        },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1
        {
            .name = "fimc1",
            .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1 * SZ_1K,
            .start = 0
        },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1
        {
            .name = "mfc1",
            .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1 * SZ_1K,
            { .alignment = 1 << 17 },
        },
#endif
#if !defined(CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION) && \
        defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0)
        {
            .name = "mfc0",
            .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0 * SZ_1K,
            { .alignment = 1 << 17 },
        },
#endif
        {
            .size = 0
        },
    };

    struct cma_region *regions_secure = NULL;

    static const char map[] __initconst =
        "android_pmem.0=pmem;android_pmem.1=pmem_gpu1;"
        "s3cfb.0/fimd=fimd;exynos4-fb.0/fimd=fimd;"
        "s3c-fimc.0=fimc0;s3c-fimc.1=fimc1;s3c-fimc.2=fimc2;s3c-fimc.3=fimc3;"
        "exynos4210-fimc.0=fimc0;exynos4210-fimc.1=fimc1;exynos4210-fimc.2=fimc2;exynos4210-fimc.3=fimc3;"
#ifdef CONFIG_VIDEO_MFC5X
        "s3c-mfc/A=mfc0,mfc-secure;"
        "s3c-mfc/B=mfc1,mfc-normal;"
        "s3c-mfc/AB=mfc;"
#endif
        "samsung-rp=srp;"
        "s5p-jpeg=jpeg;"
        "exynos4-fimc-is/f=fimc_is;"
        "s5p-mixer=tv;"
        "s5p-fimg2d=fimg2d;"
        "ion-exynos=ion,fimd,fimc0,fimc1,fimc2,fimc3,fw,b1,b2;"
        "s5p-smem/mfc=mfc0,mfc-secure;"
        "s5p-smem/fimc=fimc3;"
        "s5p-smem/mfc-shm=mfc1,mfc-normal;";

    cma_set_defaults(NULL, map);

    exynos4_cma_region_reserve(regions, regions_secure);
}
#endif

/* LCD Backlight data */
static struct samsung_bl_gpio_info smdk4x12_bl_gpio_info = {
    .no = EXYNOS4_GPD0(1),
    .func = S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data smdk4x12_bl_data = {
    .pwm_id = 1,
};

static void __init smdk4x12_map_io(void)
{
    clk_xusbxti.rate = 24000000;
    s5p_init_io(NULL, 0, S5P_VA_CHIPID);
    s3c24xx_init_clocks(24000000);
    s3c24xx_init_uarts(smdk4x12_uartcfgs, ARRAY_SIZE(smdk4x12_uartcfgs));

#if defined(CONFIG_S5P_MEM_CMA)
    exynos4_reserve_mem();
#endif
}

static void __init exynos_sysmmu_init(void)
{
    ASSIGN_SYSMMU_POWERDOMAIN(fimc0, &exynos4_device_pd[PD_CAM].dev);
    ASSIGN_SYSMMU_POWERDOMAIN(fimc1, &exynos4_device_pd[PD_CAM].dev);
    ASSIGN_SYSMMU_POWERDOMAIN(fimc2, &exynos4_device_pd[PD_CAM].dev);
    ASSIGN_SYSMMU_POWERDOMAIN(fimc3, &exynos4_device_pd[PD_CAM].dev);
    ASSIGN_SYSMMU_POWERDOMAIN(jpeg, &exynos4_device_pd[PD_CAM].dev);
    ASSIGN_SYSMMU_POWERDOMAIN(mfc_l, &exynos4_device_pd[PD_MFC].dev);
    ASSIGN_SYSMMU_POWERDOMAIN(mfc_r, &exynos4_device_pd[PD_MFC].dev);
    ASSIGN_SYSMMU_POWERDOMAIN(tv, &exynos4_device_pd[PD_TV].dev);
#ifdef CONFIG_VIDEO_FIMG2D
    sysmmu_set_owner(&SYSMMU_PLATDEV(g2d_acp).dev, &s5p_device_fimg2d.dev);
#endif
#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC) || defined(CONFIG_VIDEO_MFC5X)
    sysmmu_set_owner(&SYSMMU_PLATDEV(mfc_l).dev, &s5p_device_mfc.dev);
    sysmmu_set_owner(&SYSMMU_PLATDEV(mfc_r).dev, &s5p_device_mfc.dev);
#endif
#if defined(CONFIG_VIDEO_FIMC)
    sysmmu_set_owner(&SYSMMU_PLATDEV(fimc0).dev, &s3c_device_fimc0.dev);
    sysmmu_set_owner(&SYSMMU_PLATDEV(fimc1).dev, &s3c_device_fimc1.dev);
    sysmmu_set_owner(&SYSMMU_PLATDEV(fimc2).dev, &s3c_device_fimc2.dev);
    sysmmu_set_owner(&SYSMMU_PLATDEV(fimc3).dev, &s3c_device_fimc3.dev);
#endif
#ifdef CONFIG_VIDEO_TVOUT
    sysmmu_set_owner(&SYSMMU_PLATDEV(tv).dev, &s5p_device_tvout.dev);
#endif
#ifdef CONFIG_VIDEO_JPEG_V2X
    sysmmu_set_owner(&SYSMMU_PLATDEV(jpeg).dev, &s5p_device_jpeg.dev);
#endif
}

// wait for i2c5 bus idle before software reset
extern int wait_for_i2c_idle(struct platform_device *pdev);
static void smdk4x12_power_off(void)
{
    /* 这里需要确认，原理图中的  MD_PWON 是 GPC0_0 */
#if 0
    gpio_set_value(EXYNOS4_GPC0(0), 0);// MD_PWON low
#else
    gpio_set_value(EXYNOS4_GPX3(3), 0);// MD_PWON low
#endif

#if 0
    msleep(10);
    gpio_set_value(EXYNOS4_GPC0(2), 0);// MD_RSTN low
    gpio_set_value(EXYNOS4_GPL2(1), 0);// MD_RESETBB low
    msleep(10);	   
#endif

    /* PS_HOLD --> Output Low */
    printk(KERN_EMERG "%s : setting GPIO_PDA_PS_HOLD low.\n", __func__);
    /* PS_HOLD output High --> Low  PS_HOLD_CONTROL, R/W, 0xE010_E81C */
    writel(0x5200, S5P_PS_HOLD_CONTROL);

    while(1);

    printk(KERN_EMERG "%s : should not reach here!\n", __func__);
}

extern void (*s3c_config_sleep_gpio_table)(void);
#include <plat/gpio-core.h>

int s3c_gpio_slp_cfgpin(unsigned int pin, unsigned int config)
{
    struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
    void __iomem *reg;
    unsigned long flags;
    int offset;
    u32 con;
    int shift;

    if (!chip)
        return -EINVAL;

    if ((pin >= EXYNOS4_GPX0(0)) && (pin <= EXYNOS4_GPX3(7)))
        return -EINVAL;

    if (config > S3C_GPIO_SLP_PREV)
        return -EINVAL;

    reg = chip->base + 0x10;

    offset = pin - chip->chip.base;
    shift = offset * 2;

    local_irq_save(flags);

    con = __raw_readl(reg);
    con &= ~(3 << shift);
    con |= config << shift;
    __raw_writel(con, reg);

    local_irq_restore(flags);
    return 0;
}

int s3c_gpio_slp_setpull_updown(unsigned int pin, unsigned int config)
{
    struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
    void __iomem *reg;
    unsigned long flags;
    int offset;
    u32 con;
    int shift;

    if (!chip)
        return -EINVAL;

    if ((pin >= EXYNOS4_GPX0(0)) && (pin <= EXYNOS4_GPX3(7)))
        return -EINVAL;

    if (config > S3C_GPIO_PULL_UP)
        return -EINVAL;

    reg = chip->base + 0x14;

    offset = pin - chip->chip.base;
    shift = offset * 2;

    local_irq_save(flags);

    con = __raw_readl(reg);
    con &= ~(3 << shift);
    con |= config << shift;
    __raw_writel(con, reg);

    local_irq_restore(flags);

    return 0;
}

static void config_sleep_gpio_table(int array_size, unsigned int (*gpio_table)[3])
{
    u32 i, gpio;

    for (i = 0; i < array_size; i++) {
        gpio = gpio_table[i][0];
        s3c_gpio_slp_cfgpin(gpio, gpio_table[i][1]);
        s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][2]);
    }
}

/*sleep gpio table for TC4*/
static unsigned int tc4_sleep_gpio_table[][3] = {
    { EXYNOS4_GPA0(0),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE}, //BT_TXD
    { EXYNOS4_GPA0(1),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE}, //BT_RXD
    { EXYNOS4_GPA0(2),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE}, //BT_RTS
    { EXYNOS4_GPA0(3),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE}, //BT_CTS

    { EXYNOS4_GPA0(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //AC100_TXD,SMM6260
    { EXYNOS4_GPA0(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //AC100_RXD
    { EXYNOS4_GPA0(6),	S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //AC100_RTS
    { EXYNOS4_GPA0(7),	S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, //AC100_CTS

    { EXYNOS4_GPA1(0),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE}, //DEBUG
    { EXYNOS4_GPA1(1),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE}, //DEBUG
    { EXYNOS4_GPA1(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},  //I2C_SDA3
    { EXYNOS4_GPA1(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //I2C_SCL3

#ifdef CONFIG_TC4_DVT
    { EXYNOS4_GPA1(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //GPS_TXD
    { EXYNOS4_GPA1(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN}, //GPS_RXD
#endif

    { EXYNOS4_GPB(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //I2C_SDA4
    { EXYNOS4_GPB(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //I2C_SCL4
    { EXYNOS4_GPB(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //I2C_SDA5
    { EXYNOS4_GPB(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //I2C_SCL5
    { EXYNOS4_GPB(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN}, //GPS_RST
    { EXYNOS4_GPB(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN}, //PMIC_SET1
    { EXYNOS4_GPB(6),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN}, //PMIC_SET2
    { EXYNOS4_GPB(7),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN}, //PMIC_SET3

    //{ EXYNOS4_GPC0(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN}, //MD_PWON
    { EXYNOS4_GPX3(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN}, //MD_PWON
    { EXYNOS4_GPC0(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN}, //VLED_ON
    { EXYNOS4_GPC0(2),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE}, //MD_RSTN
    { EXYNOS4_GPC0(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN}, //AP_SLEEP
    { EXYNOS4_GPC0(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN}, //AP_WAKEUP_MD

    { EXYNOS4_GPC1(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //UART_SW  config as hp  out1??
    { EXYNOS4_GPC1(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //LED_EN18
    { EXYNOS4_GPC1(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN}, //VLED_EN
    { EXYNOS4_GPC1(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //I2C6_SDA
    { EXYNOS4_GPC1(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //I2C6_SCL

    { EXYNOS4_GPD0(0),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE}, // MOTOR-PWM
    { EXYNOS4_GPD0(1),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE}, //XPWMOUT1
    { EXYNOS4_GPD0(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},  //I2C7_SDA
    { EXYNOS4_GPD0(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //I2C7_SCL

    { EXYNOS4_GPD1(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //I2C0_SDA
    { EXYNOS4_GPD1(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //I2C0_SCL
    { EXYNOS4_GPD1(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //I2C1_SDA
    { EXYNOS4_GPD1(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE}, //I2C1_SCL

    { EXYNOS4_GPF0(0),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//LCD_HSYNC
    { EXYNOS4_GPF0(1),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//LCD_VSYNC
    { EXYNOS4_GPF0(2),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//LCD_VDEN
    { EXYNOS4_GPF0(3),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//LCD_VCLK
    { EXYNOS4_GPF0(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//CAM2M_RST
    { EXYNOS4_GPF0(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//CAM2M_PWDN
    { EXYNOS4_GPF0(6),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//LCD_D2
    { EXYNOS4_GPF0(7),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//LCD_D3

    { EXYNOS4_GPF1(0),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//LCD_D4
    { EXYNOS4_GPF1(1),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//LCD_D5
    { EXYNOS4_GPF1(2),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D6
    { EXYNOS4_GPF1(3),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D7
    { EXYNOS4_GPF1(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//CAM5M_RST
    { EXYNOS4_GPF1(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//CAM5M_PWDN
    { EXYNOS4_GPF1(6),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D10
    { EXYNOS4_GPF1(7),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D11

    { EXYNOS4_GPF2(0),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D12
    { EXYNOS4_GPF2(1),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D13
    { EXYNOS4_GPF2(2),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D14
    { EXYNOS4_GPF2(3),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D15
    { EXYNOS4_GPF2(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NC
    { EXYNOS4_GPF2(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NC
    { EXYNOS4_GPF2(6),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D18
    { EXYNOS4_GPF2(7),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D19

    { EXYNOS4_GPF3(0),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D20
    { EXYNOS4_GPF3(1),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D21
    { EXYNOS4_GPF3(2),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D22
    { EXYNOS4_GPF3(3),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//D23
    { EXYNOS4_GPF3(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NC
    { EXYNOS4_GPF3(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//MD_G15

    { EXYNOS4212_GPJ0(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//CAM_CLK
    { EXYNOS4212_GPJ0(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//CAM_VSYNC
    { EXYNOS4212_GPJ0(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//CAM_HREF
    { EXYNOS4212_GPJ0(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//DATA0
    { EXYNOS4212_GPJ0(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//DATA1
    { EXYNOS4212_GPJ0(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//DATA2
    { EXYNOS4212_GPJ0(6),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//DATA3
    { EXYNOS4212_GPJ0(7),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//DATA4

    { EXYNOS4212_GPJ1(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//DATA5
    { EXYNOS4212_GPJ1(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//DATA6
    { EXYNOS4212_GPJ1(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//DATA7
    { EXYNOS4212_GPJ1(3),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//CAM_CLK_OUT
    { EXYNOS4212_GPJ1(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NC

    { EXYNOS4_GPK0(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//eMMC_CLK
    { EXYNOS4_GPK0(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//eMMC_CMD
    { EXYNOS4_GPK0(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//eMMC_CDn
    { EXYNOS4_GPK0(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//eMMC_DATA0
    { EXYNOS4_GPK0(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//eMMC_DATA1
    { EXYNOS4_GPK0(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//eMMC_DATA2
    { EXYNOS4_GPK0(6),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//eMMC_DATA3

#ifdef CONFIG_TC4_DVT
    { EXYNOS4_GPK1(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//ANX7805_PD
    { EXYNOS4_GPK1(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//VDD50_EN
#endif

    { EXYNOS4_GPK1(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NC
    { EXYNOS4_GPK1(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//eMMC_DATA4
    { EXYNOS4_GPK1(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//eMMC_DATA5
    { EXYNOS4_GPK1(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//eMMC_DATA6
    { EXYNOS4_GPK1(6),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//eMMC_DATA7

    { EXYNOS4_GPK2(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//TF_CLK
    { EXYNOS4_GPK2(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//TF_CMD
    { EXYNOS4_GPK2(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//ANX7805_RSTN
    { EXYNOS4_GPK2(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//TF_DATA0
    { EXYNOS4_GPK2(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//TF_DATA1
    { EXYNOS4_GPK2(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//TF_DATA2
    { EXYNOS4_GPK2(6),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//TF_DATA3

    { EXYNOS4_GPK3(0),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},	//WIFI_CLK
    { EXYNOS4_GPK3(1),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},	//WIFI_CMD
    { EXYNOS4_GPK3(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//HUB_CONNECT
    { EXYNOS4_GPK3(3),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},	//WIFI_DATA0
    { EXYNOS4_GPK3(4),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},	//WIFI_DATA1
    { EXYNOS4_GPK3(5),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},	//WIFI_DATA2
    { EXYNOS4_GPK3(6),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},	//WIFI_DATA3

    { EXYNOS4_GPL0(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//BUCK6_EN

#ifdef CONFIG_TC4_DVT
    { EXYNOS4_GPL0(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//6260_GPIO3
#endif

    { EXYNOS4_GPL0(2), /* S3C_GPIO_SLP_PREV*/S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE},	//TP1_EN
    { EXYNOS4_GPL0(3),  S3C_GPIO_SLP_OUT1,	S3C_GPIO_PULL_NONE},	//NFC_EN1  out1

    { EXYNOS4_GPL0(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//CHG_EN
    { EXYNOS4_GPL0(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NO THIS PIN
    { EXYNOS4_GPL0(6),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//HDMI_IIC_EN
    { EXYNOS4_GPL0(7),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NC

    { EXYNOS4_GPL1(0),  S3C_GPIO_SLP_OUT0,	S3C_GPIO_PULL_NONE}, 	//LVDS_PWDN  out0
    { EXYNOS4_GPL1(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NC
    { EXYNOS4_GPL1(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NC

    { EXYNOS4_GPL2(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//KP_COL0
    { EXYNOS4_GPL2(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//MD_RESETBB
    { EXYNOS4_GPL2(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//HUB_RESET
    { EXYNOS4_GPL2(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//NFC_SCL
    { EXYNOS4_GPL2(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_NONE},	//NFC_SDA
    { EXYNOS4_GPL2(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NFC_GPIO4

#ifdef CONFIG_TC4_DVT
    //GPM4(2) --ISP_SCL1
    //GPM4(3)--ISP_SDA1
    //GPM3(5)--PMIC_DS2
    //GPM3(6)--PMIC_DS3
    //GPM3(7)--PMIC_DS4
#endif

    { EXYNOS4_GPY0(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NC
    { EXYNOS4_GPY0(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NC

    { EXYNOS4_GPY0(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY0(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY0(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY0(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */

    { EXYNOS4_GPY1(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY1(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY1(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY1(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */

    { EXYNOS4_GPY2(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY2(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY2(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY2(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY2(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY2(5),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */

    { EXYNOS4_GPY3(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NC
    { EXYNOS4_GPY3(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	//NC
    { EXYNOS4_GPY3(2),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* MHL_SCL_1.8V */
    { EXYNOS4_GPY3(3),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
    { EXYNOS4_GPY3(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
    { EXYNOS4_GPY3(5),  S3C_GPIO_SLP_INPUT,  S3C_GPIO_PULL_DOWN},
    { EXYNOS4_GPY3(6),  S3C_GPIO_SLP_INPUT,  S3C_GPIO_PULL_DOWN},
    { EXYNOS4_GPY3(7),  S3C_GPIO_SLP_INPUT,  S3C_GPIO_PULL_DOWN},

    { EXYNOS4_GPY4(0),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
    { EXYNOS4_GPY4(1),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
    { EXYNOS4_GPY4(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},
    { EXYNOS4_GPY4(3),  S3C_GPIO_SLP_INPUT,  S3C_GPIO_PULL_DOWN},
    { EXYNOS4_GPY4(4),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY4(5),  S3C_GPIO_SLP_INPUT,  S3C_GPIO_PULL_DOWN},
    { EXYNOS4_GPY4(6),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},
    { EXYNOS4_GPY4(7),  S3C_GPIO_SLP_INPUT,	S3C_GPIO_PULL_DOWN},

    { EXYNOS4_GPY5(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY5(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY5(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY5(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY5(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY5(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY5(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY5(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */

    { EXYNOS4_GPY6(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY6(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY6(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY6(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY6(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY6(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY6(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */
    { EXYNOS4_GPY6(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	/* NC */

    { EXYNOS4_GPZ(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	//I2S0_SCLK
    { EXYNOS4_GPZ(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN},	//I2S0_CDCLK
    { EXYNOS4_GPZ(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //I2S0_LRCK
    { EXYNOS4_GPZ(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //I2S0_SDI
    { EXYNOS4_GPZ(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //I2S0_SDO0
    { EXYNOS4_GPZ(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, //WIFI_PWDN
    { EXYNOS4_GPZ(6),  S3C_GPIO_SLP_PREV,	S3C_GPIO_PULL_NONE},	//BT_RST
};

static void tc4_config_sleep_gpio_table(void)
{
    config_sleep_gpio_table(ARRAY_SIZE(tc4_sleep_gpio_table),
            tc4_sleep_gpio_table); ///yulu
}

#define SMDK4412_REV_0_0_ADC_VALUE 0
#define SMDK4412_REV_0_1_ADC_VALUE 443
int samsung_board_rev;

static int get_samsung_board_rev(void)
{
    int		ret = 0;
    ret = 1;
    return ret;
}

/* 在 arch_initcall 中调用， 见 customize_machine  */
static void __init smdk4x12_machine_init(void)
{
    /* .config 中已定义 CONFIG_S3C64XX_DEV_SPI */
#ifdef CONFIG_S3C64XX_DEV_SPI
    unsigned int gpio;
    struct clk *sclk = NULL;
    struct clk *prnt = NULL;
    struct device *spi2_dev = &exynos_device_spi2.dev;
#endif

    /* 机器特定的掉电处理函数，被平台掉电处理函数调用 */
    pm_power_off = smdk4x12_power_off;

    /* 休眠，gpio处理，休眠时设置gpio进入的状态 */
    s3c_config_sleep_gpio_table = tc4_config_sleep_gpio_table;

    /* 单板版本，无实际作用 */
    samsung_board_rev = get_samsung_board_rev();

    /* .config CONFIG_EXYNOS_DEV_PD  CONFIG_PM_RUNTIME  均定义了 */
#if defined(CONFIG_EXYNOS_DEV_PD) && defined(CONFIG_PM_RUNTIME)
    exynos_pd_disable(&exynos4_device_pd[PD_MFC].dev);
    exynos_pd_disable(&exynos4_device_pd[PD_G3D].dev);
    exynos_pd_disable(&exynos4_device_pd[PD_LCD0].dev);
    exynos_pd_disable(&exynos4_device_pd[PD_CAM].dev);
    exynos_pd_disable(&exynos4_device_pd[PD_TV].dev);
    exynos_pd_disable(&exynos4_device_pd[PD_GPS].dev);
    exynos_pd_disable(&exynos4_device_pd[PD_GPS_ALIVE].dev);
    exynos_pd_disable(&exynos4_device_pd[PD_ISP].dev);
#endif

    s3c_i2c0_set_platdata(NULL);
    i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));

    s3c_i2c1_set_platdata(NULL);
    i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

    s3c_i2c2_set_platdata(NULL);
    i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));

    init_lcd_type();
    get_lcd_type();
    setup_ft5x_width_height();

    s3c_i2c3_set_platdata(NULL);
    i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));

    s3c_i2c4_set_platdata(NULL);
    i2c_register_board_info(4, i2c_devs4, ARRAY_SIZE(i2c_devs4));

    s3c_i2c5_set_platdata(NULL);
    i2c_register_board_info(5, i2c_devs5, ARRAY_SIZE(i2c_devs5));

    s3c_i2c6_set_platdata(NULL);
    i2c_register_board_info(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));

    s3c_i2c7_set_platdata(NULL);
    i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7));

#ifdef CONFIG_ANDROID_PMEM
    android_pmem_set_platdata();
#endif
#ifdef CONFIG_FB_S5P
    s3cfb_set_platdata(NULL);
#ifdef CONFIG_EXYNOS_DEV_PD
    s3c_device_fb.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
#endif
#ifdef CONFIG_USB_EHCI_S5P
    smdk4x12_ehci_init();
#endif
#ifdef CONFIG_USB_GADGET
    smdk4x12_usbgadget_init();
#endif

    samsung_bl_set(&smdk4x12_bl_gpio_info, &smdk4x12_bl_data);

#ifdef CONFIG_S3C_DEV_HSMMC2
    s3c_sdhci2_set_platdata(&smdk4x12_hsmmc2_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
    s3c_sdhci3_set_platdata(&smdk4x12_hsmmc3_pdata);
#endif
#ifdef CONFIG_S5P_DEV_MSHC
    s3c_mshci_set_platdata(&exynos4_mshc_pdata);
#endif
#ifdef CONFIG_VIDEO_FIMC
    s3c_fimc0_set_platdata(&fimc_plat);
    s3c_fimc1_set_platdata(&fimc_plat);
    s3c_fimc2_set_platdata(NULL);
    s3c_fimc3_set_platdata(NULL);
#ifdef CONFIG_EXYNOS_DEV_PD
    s3c_device_fimc0.dev.parent = &exynos4_device_pd[PD_CAM].dev;
    s3c_device_fimc1.dev.parent = &exynos4_device_pd[PD_CAM].dev;
    s3c_device_fimc2.dev.parent = &exynos4_device_pd[PD_CAM].dev;
    s3c_device_fimc3.dev.parent = &exynos4_device_pd[PD_CAM].dev;
#endif
#ifdef CONFIG_VIDEO_FIMC_MIPI
    s3c_csis0_set_platdata(NULL);
    s3c_csis1_set_platdata(NULL);
#ifdef CONFIG_EXYNOS_DEV_PD
    s3c_device_csis0.dev.parent = &exynos4_device_pd[PD_CAM].dev;
    s3c_device_csis1.dev.parent = &exynos4_device_pd[PD_CAM].dev;
#endif
#endif

#if defined(CONFIG_ITU_B) || defined(CONFIG_CSI_D) \
    || defined(CONFIG_S5K3H1_CSI_D) || defined(CONFIG_S5K3H2_CSI_D) \
    || defined(CONFIG_VIDEO_OV5640) \
    || defined(CONFIG_S5K6A3_CSI_D)
    smdk4x12_cam1_reset(1);
#endif
#endif /* CONFIG_VIDEO_FIMC */

#if defined(CONFIG_VIDEO_TVOUT)
    s5p_hdmi_hpd_set_platdata(&hdmi_hpd_data);
    s5p_hdmi_cec_set_platdata(&hdmi_cec_data);
#ifdef CONFIG_EXYNOS_DEV_PD
    s5p_device_tvout.dev.parent = &exynos4_device_pd[PD_TV].dev;
    exynos4_device_pd[PD_TV].dev.parent= &exynos4_device_pd[PD_LCD0].dev;
#endif
#endif

#if	defined(CONFIG_VIDEO_JPEG_V2X) || defined(CONFIG_VIDEO_JPEG)
#ifdef CONFIG_EXYNOS_DEV_PD
    s5p_device_jpeg.dev.parent = &exynos4_device_pd[PD_CAM].dev;
    exynos4_jpeg_setup_clock(&s5p_device_jpeg.dev, 160000000);
#endif
#endif

#ifdef CONFIG_ION_EXYNOS
    exynos_ion_set_platdata();
#endif

#if defined(CONFIG_VIDEO_MFC5X) || defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
#ifdef CONFIG_EXYNOS_DEV_PD
    s5p_device_mfc.dev.parent = &exynos4_device_pd[PD_MFC].dev;
#endif
    if (soc_is_exynos4412() && samsung_rev() >= EXYNOS4412_REV_1_0)
        exynos4_mfc_setup_clock(&s5p_device_mfc.dev, 200 * MHZ);
    else
        exynos4_mfc_setup_clock(&s5p_device_mfc.dev, 267 * MHZ);
#endif

#ifdef CONFIG_VIDEO_FIMG2D
    s5p_fimg2d_set_platdata(&fimg2d_data);
#endif

#ifdef CONFIG_SAMSUNG_DEV_KEYPAD
    samsung_keypad_set_platdata(&smdk4x12_keypad_data);
#endif

#ifdef CONFIG_USB_NET_DM9620
    dm9620_reset();
#endif

    exynos_sysmmu_init();

    smdk4x12_gpio_power_init();

    platform_add_devices(smdk4x12_devices, ARRAY_SIZE(smdk4x12_devices));
    if (soc_is_exynos4412())
        platform_add_devices(smdk4412_devices, ARRAY_SIZE(smdk4412_devices));

#ifdef CONFIG_S3C64XX_DEV_SPI
    sclk = clk_get(spi2_dev, "dout_spi2");
    if (IS_ERR(sclk))
        dev_err(spi2_dev, "failed to get sclk for SPI-2\n");
    prnt = clk_get(spi2_dev, "mout_mpll_user");
    if (IS_ERR(prnt))
        dev_err(spi2_dev, "failed to get prnt\n");
    if (clk_set_parent(sclk, prnt))
        printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
                prnt->name, sclk->name);

    clk_set_rate(sclk, 800 * 1000 * 1000);
    clk_put(sclk);
    clk_put(prnt);

    if (!gpio_request(EXYNOS4_GPC1(2), "SPI_CS2")) {
        gpio_direction_output(EXYNOS4_GPC1(2), 1);
        s3c_gpio_cfgpin(EXYNOS4_GPC1(2), S3C_GPIO_SFN(1));
        s3c_gpio_setpull(EXYNOS4_GPC1(2), S3C_GPIO_PULL_UP);
        exynos_spi_set_info(2, EXYNOS_SPI_SRCCLK_SCLK,
                ARRAY_SIZE(spi2_csi));
    }

    for (gpio = EXYNOS4_GPC1(1); gpio < EXYNOS4_GPC1(5); gpio++)
        s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);

    spi_register_board_info(spi2_board_info, ARRAY_SIZE(spi2_board_info));
#endif

#ifdef CONFIG_BUSFREQ_OPP
    dev_add(&busfreq, &exynos4_busfreq.dev);
    ppmu_init(&exynos_ppmu[PPMU_DMC0], &exynos4_busfreq.dev);
    ppmu_init(&exynos_ppmu[PPMU_DMC1], &exynos4_busfreq.dev);
    ppmu_init(&exynos_ppmu[PPMU_CPU], &exynos4_busfreq.dev);
#endif

    register_reboot_notifier(&exynos4_reboot_notifier);
}


/* .config 中定义了 CONFIG_TC4_ICS */
/* uboot 传进来的 mach_id 是 2838， 对应的是 MACH_TYPE_SMDKC210 
 * 但 kernel 的 .config 中配置的是 CONFIG_MACH_SMDK4X12 
 * 对应的是 MACH_TYPE_SMDK4X12 , 
 * 通过打印分析，在调用 setup.c setup_machine_tags 时，
 * 传入的 machine_arch_type = 3698 
 * 3698 对应的是 MACH_TYPE_SMDK4212 , 也就是下面的第一个
 * 再分析 include/generated/mach-types.h 文件中宏定义 
 * 发现 MACH_TYPE_SMDK4212 与 MACH_TYPE_SMDK4X12 的值是相同的。。。
 * 而且下面的这两个宏，其中赋的成员是完全一致的，也就是用那个
 * machine 都是一样的 */
/* 
 * 下面这些是 include/generated/mach-types.h 中摘的
 * 需要留意 machine_arch_type 的使用
 * config_for_linux_scp_elite 配置文件中，定义了 CONFIG_MACH_SMDK4X12 
 * 后面的如果定义了 machine_arch_type 则重新定义为 __machine_arch_type
 * __machine_arch_type 这个是一个变量，赋值为 uboot 传入的 mach_id
 * 而没有定义 machine_arch_type , 则 赋值为 include/generated/mach-types.h
 * 中的 MACH_TYPE_XXXX 
 *
#ifdef CONFIG_MACH_SMDK4212                                              
# ifdef machine_arch_type                                                
#  undef machine_arch_type                                               
#  define machine_arch_type __machine_arch_type                          
# else                                                                   
#  define machine_arch_type MACH_TYPE_SMDK4212                           
# endif                                                                  
# define machine_is_smdk4212()  (machine_arch_type == MACH_TYPE_SMDK4212)
#else                                                                    
# define machine_is_smdk4212()  (0)                                      
#endif                                                                   

#ifdef CONFIG_MACH_SMDK4X12                                              
# ifdef machine_arch_type                                                
#  undef machine_arch_type                                               
#  define machine_arch_type __machine_arch_type                          
# else                                                                   
#  define machine_arch_type MACH_TYPE_SMDK4X12                           
# endif                                                                  
# define machine_is_smdk4x12()  (machine_arch_type == MACH_TYPE_SMDK4X12)
#else                                                                    
# define machine_is_smdk4x12()  (0)                                      
#endif                                                                   

*/
#ifdef CONFIG_TC4_ICS
MACHINE_START(SMDK4212, "SMDK4X12")
    .boot_params	= S5P_PA_SDRAM + 0x100,
    .init_irq	    = exynos4_init_irq,
    .map_io		    = smdk4x12_map_io,
    .init_machine	= smdk4x12_machine_init,
    .timer		    = &exynos4_timer,
#if defined(CONFIG_KERNEL_PANIC_DUMP)		//mj for panic-dump
    .reserve		= reserve_panic_dump_area,
#endif
MACHINE_END

MACHINE_START(SMDK4412, "SMDK4X12")
    .boot_params	= S5P_PA_SDRAM + 0x100,
    .init_irq	    = exynos4_init_irq,
    .map_io		    = smdk4x12_map_io,
    .init_machine	= smdk4x12_machine_init,
    .timer		    = &exynos4_timer,
#if defined(CONFIG_KERNEL_PANIC_DUMP)		//mj for panic-dump
    .reserve		= reserve_panic_dump_area,
#endif
MACHINE_END
#endif


