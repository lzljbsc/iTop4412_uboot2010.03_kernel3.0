
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <asm/io.h>
#include <movi.h>
#include <stdio_dev.h>
#include <timestamp.h>
#include <version.h>
#include <serial.h>
#include <nand.h>  // TODO: 移除
#include <onenand_uboot.h> // TODO: 移除
#include <s5pc210.h>
#include <mmc.h>

#undef DEBUG

extern int recovery_preboot(void);

DECLARE_GLOBAL_DATA_PTR;

ulong monitor_flash_len;

const char version_string[] =
	U_BOOT_VERSION" (" U_BOOT_DATE " - " U_BOOT_TIME ")"CONFIG_IDENT_STRING;

/************************************************************************
 * Coloured LED functionality
 ************************************************************************
 * May be supplied by boards if desired
 */
void inline __coloured_LED_init (void) {}
void coloured_LED_init (void) __attribute__((weak, alias("__coloured_LED_init")));
void inline __red_LED_on (void) {}
void red_LED_on (void) __attribute__((weak, alias("__red_LED_on")));
void inline __red_LED_off(void) {}
void red_LED_off(void) __attribute__((weak, alias("__red_LED_off")));
void inline __green_LED_on(void) {}
void green_LED_on(void) __attribute__((weak, alias("__green_LED_on")));
void inline __green_LED_off(void) {}
void green_LED_off(void) __attribute__((weak, alias("__green_LED_off")));
void inline __yellow_LED_on(void) {}
void yellow_LED_on(void) __attribute__((weak, alias("__yellow_LED_on")));
void inline __yellow_LED_off(void) {}
void yellow_LED_off(void) __attribute__((weak, alias("__yellow_LED_off")));
void inline __blue_LED_on(void) {}
void blue_LED_on(void) __attribute__((weak, alias("__blue_LED_on")));
void inline __blue_LED_off(void) {}
void blue_LED_off(void) __attribute__((weak, alias("__blue_LED_off")));


static int init_baudrate (void)
{
    /* baudrate 在环境变量中， CONFIG_BAUDRATE 115200 */
	char tmp[64];	/* long enough for environment variables */
	int i = getenv_r ("baudrate", tmp, sizeof (tmp));
	gd->bd->bi_baudrate = gd->baudrate = (i > 0)
			? (int) simple_strtoul (tmp, NULL, 10)
			: CONFIG_BAUDRATE;

	return (0);
}

static int off_charge(void)
{
	volatile int value = 0;
	int i,j;
	int val;

	/*---only evt use this---*/
	val = readl(GPX2PUD);
	val &= ~(0x3<<12); //dok,pullup/down disable
	writel(val,GPX2PUD);
	
	/*---volume key setting---*/
	__REG(GPL2CON) = __REG(GPL2CON)|0x1<<0;
	__REG(GPL2DAT) = __REG(GPL2DAT)|0x1<<0;//output 1

	__REG(GPX2CON) = __REG(GPX2CON)&~0xff; //gpx2.0,gpx2.1

	val = readl(GPX2PUD);
	val &= ~(0xf);
	val |= (0x5);
	writel(val,GPX2PUD);

	/*---gpx2.7 cok ---setting--*/
	val = readl(GPX2CON);
	val &= ~(0xf<<28);
	writel(val,GPX2CON);
	
	val = readl(GPX2PUD);
	val &= ~(0x3<<14); //cok gpx2.7,pullup/down disable
	writel(val,GPX2PUD);

	/*---this don't need ---*/
	val = readl(GPX1PUD);
	val &= ~(0x3<<10); //uok,pullup/down disable
	writel(val,GPX1PUD);

	/*---on key setting gpx0.2--*/
	val = readl(GPX0PUD);
	val &= ~(0x3<<4); //on_key,pullup/down disable
	writel(val,GPX0PUD);
	
	return 0;
}

static int display_banner (void)
{
	printf ("\n\n%s\n\n", version_string);
	debug ("U-Boot code: %08lX -> %08lX  BSS: -> %08lX\n",
	       _armboot_start, _bss_start, _bss_end);

	return (0);
}

static int display_dram_config (void)
{
	int i;

	ulong size = 0;

	for (i=0; i<CONFIG_NR_DRAM_BANKS; i++) {
		size += gd->bd->bi_dram[i].size;
	}
	size += 0x100000;

  	puts("DRAM:	");
	print_size(size, "\n");

	return (0);
}

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependent #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

int print_cpuinfo (void);

/* 初始化序列，返回值不为0，则认为失败，进行停机 */
init_fnc_t *init_sequence[] = {
	board_init,		    /* basic board dependent setup */
	interrupt_init,		/* set up exceptions */
	env_init,		    /* initialize environment */
	init_baudrate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,		/* stage 1 init of console */
	off_charge,		    // xiebin.wang @ 20110531,for charger&power off device.
	display_banner,		/* say that we are here */
	print_cpuinfo,		/* display cpu info (and speed) */
	checkboard,		    /* display board info */
	dram_init,		    /* configure available RAM banks */
	display_dram_config,
	NULL,
};

/* uboot C语言入口函数 */
void start_armboot (void)
{
	init_fnc_t **init_fnc_ptr;
	char *s;
	int mmc_exist = 0;

    /* _armboot_start 为uboot代码起始地址，根据start.s 中代码，该值为
     * 0x43e00010 (链接地址为 0xc3e00000, 所以实际为 0xc3e00010)
     * CONFIG_SYS_MALLOC_LEN 0x104000
     * 这里是在 uboot镜像的下方内存处预留了一部分空间用于 gd_t 结构体 */
	/* Pointer is writable since we allocated a register for it */
	gd = (gd_t*)(_armboot_start - CONFIG_SYS_MALLOC_LEN - sizeof(gd_t));
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");

    /* 从 gd 空间继续向下预留一段内存用于 bd_t 结构体
     * 并将 gd->bd 指向 bd_t 结构体 */
	memset ((void*)gd, 0, sizeof (gd_t));
	gd->bd = (bd_t*)((char*)gd - sizeof(bd_t));
	memset (gd->bd, 0, sizeof (bd_t));

    /* monitor_flash_len 是 uboot 代码段长度 */
    /* 这个长度比 u-boot.bin 文件小 16字节, 即 start.s 文件中最开始的16字节 */
	monitor_flash_len = _bss_start - _armboot_start;

    /* 按照初始化序列依次调用初始化函数，如失败则停止(hang) */
	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			hang ();
		}
	}
	
	/* armboot_start is defined in the board-specific linker script */
	mem_malloc_init(_armboot_start - CONFIG_SYS_MALLOC_LEN,
			CONFIG_SYS_MALLOC_LEN);

	puts ("MMC:   ");
	mmc_exist = mmc_initialize (gd->bd);
	if (mmc_exist != 0)
	{
		puts ("0 MB\n");
	}

	/* initialize environment */
	env_relocate ();

	/* IP Address */
	gd->bd->bi_ip_addr = getenv_IPaddr ("ipaddr");

	stdio_init ();	/* get the devices list going. */

	jumptable_init ();

	console_init_r ();	/* fully init console as a device */

	/* enable exceptions */
	enable_interrupts ();

	/* Initialize from environment */
	if ((s = getenv ("loadaddr")) != NULL) {
		load_addr = simple_strtoul (s, NULL, 16);
	}

	board_late_init ();

	/* main_loop() can return to retry autoboot, if so just run it again. */
	recovery_preboot();

	for (;;) {
		main_loop ();
	}

	/* NOTREACHED - no way out of command loop except booting */
}

void hang (void)
{
	puts ("### ERROR ### Please RESET the board ###\n");
	for (;;);
}
