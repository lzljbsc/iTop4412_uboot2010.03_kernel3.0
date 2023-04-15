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

void jumptable_init(void)
{
	gd->jt = malloc(XF_MAX * sizeof(void *));
#include <_exports.h>
}
