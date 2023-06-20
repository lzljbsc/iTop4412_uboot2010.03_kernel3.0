#include <common.h>
#include <command.h>
#include <image.h>
#include <u-boot/zlib.h>
#include <asm/byteorder.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG)
static void setup_start_tag (bd_t *bd);

# ifdef CONFIG_SETUP_MEMORY_TAGS
static void setup_memory_tags (bd_t *bd);
# endif
static void setup_commandline_tag (bd_t *bd, char *commandline);

# ifdef CONFIG_INITRD_TAG
static void setup_initrd_tag (bd_t *bd, ulong initrd_start,
			      ulong initrd_end);
# endif
static void setup_end_tag (bd_t *bd);

static struct tag *params;
#endif /* CONFIG_SETUP_MEMORY_TAGS || CONFIG_CMDLINE_TAG || CONFIG_INITRD_TAG */

/* 启动linux的函数 */
/* 调用方式     boot_fn(0, argc, argv, &images); * */
int do_bootm_linux(int flag, int argc, char *argv[], bootm_headers_t *images)
{
	bd_t	*bd = gd->bd;
	char	*s;
	int	machid = bd->bi_arch_number; /* 在 board.c 初始化中设置 */
	void	(*theKernel)(int zero, int arch, uint params);
	int	ret;

#ifdef CONFIG_CMDLINE_TAG
    /* 定义了这个宏，但在代码中并未设置 bootargs 环境变量
     * 此处是将bootargs 环境变量做为tag 传给内核 */
	char *commandline = getenv ("bootargs");
#endif

	if ((flag != 0) && (flag != BOOTM_STATE_OS_GO))
		return 1;

    /* theKernel 指向了启动地址 */
	theKernel = (void (*)(int, int, uint))images->ep;

    /* 如果有 machid 环境变量，就覆盖前面的 machid
     * 这样调试方便一些，实际没有 machid */
	s = getenv ("machid");
	if (s) {
		machid = simple_strtoul (s, NULL, 16);
		printf ("Using machid 0x%x from environment\n", machid);
	}
	
    /* ramdisk 镜像获取，在4412板中，获取失败 */
	ret = boot_get_ramdisk(argc, argv, images, IH_ARCH_ARM, 
			&(images->rd_start), &(images->rd_end));
	if(ret)
		printf("[err] boot_get_ramdisk\n");

	show_boot_progress (15);

	debug ("## Transferring control to Linux (at address %08lx) ...\n",
	       (ulong) theKernel);

    /* 组织内核参数，有多种参数方式 */
    /* 根据下面的运行结果，最终只有 setup_memory_tags 设置了
     * 内存相关的 tag， 其他的都无实质作用 */
#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG)
	setup_start_tag (bd);
#ifdef CONFIG_SETUP_MEMORY_TAGS
	setup_memory_tags (bd);
#endif
#ifdef CONFIG_CMDLINE_TAG
	setup_commandline_tag (bd, commandline);
#endif
#ifdef CONFIG_INITRD_TAG
    /* 设置 initrd , 前面的 boot_get_ramdisk 未获取到，
     * 所以这里实际是无作用的 */
	if (images->rd_start && images->rd_end)
		setup_initrd_tag (bd, images->rd_start, images->rd_end);
#endif
	setup_end_tag (bd);
#endif

	/* we assume that the kernel is in place */
	printf ("\nStarting kernel ...\n\n");

    /* 调用之前 做一些清除工作
     * 但这里好像并没有关闭mmu */
	cleanup_before_linux ();

    /* 调用内核，传入参数 */
	theKernel (0, machid, bd->bi_boot_params);
	/* does not return */

	return 1;
}


#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG)
static void setup_start_tag (bd_t *bd)
{
    /* params 是全局变量，设置参数过程中使用，
     * 永远指向将要设置参数的内存起始地址 */
	params = (struct tag *) bd->bi_boot_params;

    /* 第一个参数是 ATAG_CORE */
	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size (tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;

    /* 根据第一个 tag 的地址(params) 和大小(size) 
     * 将params 指向了后面空闲的地址 */
	params = tag_next (params);
}


#ifdef CONFIG_SETUP_MEMORY_TAGS
int nr_dram_banks = -1;
static void setup_memory_tags (bd_t *bd)
{
	int i;

    /* 设置系统内存 ATAG_MEM 
     * 按照内核文档，这个字段是必须的 */
    /* nr_dram_banks = 8
     * 在初始化内存时已经设置了 */
	for (i = 0; i < nr_dram_banks; i++) {
		params->hdr.tag = ATAG_MEM;
		params->hdr.size = tag_size (tag_mem32);

		params->u.mem.start = bd->bi_dram[i].start;
		params->u.mem.size = bd->bi_dram[i].size;

		params = tag_next (params);
	}
}
#endif /* CONFIG_SETUP_MEMORY_TAGS */


static void setup_commandline_tag (bd_t *bd, char *commandline)
{
	char *p;

    // TODO: 测试一下有 bootargs 环境变量的情况 
    /* 实际传进来的 commandline 为 NULL */
	if (!commandline)
		return;

	/* eat leading white space */
	for (p = commandline; *p == ' '; p++);

	/* skip non-existent command lines so the kernel will still
	 * use its default command line.
	 */
	if (*p == '\0')
		return;

    /* 将 commandline 拷贝到 tag 中，
     * 需要4字节长度，所以在后面增加个预留 
     * 从这里看，传递的 commandline 是没有长度限制的，
     * 前提是内存足够 */
	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size =
		(sizeof (struct tag_header) + strlen (p) + 1 + 4) >> 2;

	strcpy (params->u.cmdline.cmdline, p);

	params = tag_next (params);
}


#ifdef CONFIG_INITRD_TAG
static void setup_initrd_tag (bd_t *bd, ulong initrd_start, ulong initrd_end)
{
    /* 与其他的 tag 同样的设置方式 */
	/* an ATAG_INITRD node tells the kernel where the compressed
	 * ramdisk can be found. ATAG_RDIMG is a better name, actually.
	 */
	params->hdr.tag = ATAG_INITRD2;
	params->hdr.size = tag_size (tag_initrd);

	params->u.initrd.start = initrd_start;
	params->u.initrd.size = initrd_end - initrd_start;

	params = tag_next (params);
}
#endif /* CONFIG_INITRD_TAG */

static void setup_end_tag (bd_t *bd)
{
    /* 结束标志，表征 tag 已经没有了。。。 */
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

#endif /* CONFIG_SETUP_MEMORY_TAGS || CONFIG_CMDLINE_TAG || CONFIG_INITRD_TAG */

