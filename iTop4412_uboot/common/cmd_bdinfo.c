/*
 * Boot support
 */
#include <common.h>
#include <command.h>

DECLARE_GLOBAL_DATA_PTR;

static void print_num(const char *, ulong);

int do_bdinfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	bd_t *bd = gd->bd;

	print_num ("arch_number",	bd->bi_arch_number);
	print_num ("env_t",		(ulong)bd->bi_env);
	print_num ("boot_params",	(ulong)bd->bi_boot_params);

	for (i=0; i<CONFIG_NR_DRAM_BANKS; ++i) {
		print_num("DRAM bank",	i);
		print_num("-> start",	bd->bi_dram[i].start);
		print_num("-> size",	bd->bi_dram[i].size);
	}

	printf ("baudrate    = %d bps\n", bd->bi_baudrate);

	return 0;
}


static void print_num(const char *name, ulong value)
{
	printf ("%-12s= 0x%08lX\n", name, value);
}


/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	bdinfo,	1,	1,	do_bdinfo,
	"print Board Info structure",
	""
);
