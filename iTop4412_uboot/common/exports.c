#include <common.h>
#include <exports.h>

DECLARE_GLOBAL_DATA_PTR;

static void dummy(void)
{
}

unsigned long get_version(void)
{
	return XF_VERSION;
}

/* Reuse _exports.h with a little trickery to avoid bitrot */
#define EXPORT_FUNC(sym) gd->jt[XF_##sym] = (void *)sym;

# define install_hdlr      dummy
# define free_hdlr         dummy
# define i2c_write         dummy
# define i2c_read          dummy
# define spi_init          dummy
# define spi_setup_slave   dummy
# define spi_free_slave    dummy
# define spi_claim_bus     dummy
# define spi_release_bus   dummy
# define spi_xfer          dummy
# define forceenv          dummy

/* 这里需要结合 _exports.h 和 exports.h 两个文件
 * EXPORT_FUNC 宏在两个文件中的作用不同 
 * 综合效果是 在gd->jt 中记录很多的函数，如 
 * gd->jt[0] = get_version;
 * 而 0 也是通过宏生成的， 
 * gd->jt[XF_get_version] = get_version; 
 * 但在 uboot中仅进行了赋值，并未真正使用 */
void jumptable_init(void)
{
	gd->jt = malloc(XF_MAX * sizeof(void *));
#include <_exports.h>
}
