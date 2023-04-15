#include <common.h>
#include <command.h>

int do_echo(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i;
	int putnl = 1;

	for (i = 1; i < argc; i++) {
		char *p = argv[i], c;

		if (i > 1)
			putc(' ');
		while ((c = *p++) != '\0') {
			if (c == '\\' && *p == 'c') {
				putnl = 0;
				p++;
			} else {
				putc(c);
			}
		}
	}

	if (putnl)
		putc('\n');

	return 0;
}

U_BOOT_CMD(
	echo,	CONFIG_SYS_MAXARGS,	1,	do_echo,
	"echo args to console",
	"[args..]\n"
	"    - echo args to console; \\c suppresses newline"
);
