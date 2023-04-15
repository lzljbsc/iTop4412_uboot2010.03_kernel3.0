#include <common.h>
#include <command.h>

extern char version_string[];

int do_version(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("\n%s\n", version_string);

	return 0;
}

U_BOOT_CMD(
	version,	1,		1,	do_version,
	"print monitor version",
	""
);
