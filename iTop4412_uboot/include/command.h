/*
 *  Definitions for Command Processor
 */
#ifndef __COMMAND_H
#define __COMMAND_H

#include <config.h>

#ifndef NULL
#define NULL	0
#endif

/* Default to a width of 8 characters for help message command width */
#define CONFIG_SYS_HELP_CMD_WIDTH	8

/*
 * Monitor Command Table
 */

struct cmd_tbl_s {
	char		*name;		/* Command Name			*/
	int		maxargs;	/* maximum number of arguments	*/
	int		repeatable;	/* autorepeat allowed?		*/
					/* Implementation function	*/
	int		(*cmd)(struct cmd_tbl_s *, int, int, char *[]);
	char		*usage;		/* Usage message	(short)	*/
#ifdef	CONFIG_SYS_LONGHELP
	char		*help;		/* Help  message	(long)	*/
#endif
};

typedef struct cmd_tbl_s	cmd_tbl_t;

extern cmd_tbl_t  __u_boot_cmd_start;
extern cmd_tbl_t  __u_boot_cmd_end;


/* common/command.c */
int _do_help (cmd_tbl_t *cmd_start, int cmd_items, cmd_tbl_t * cmdtp, int
	      flag, int argc, char *argv[]);
cmd_tbl_t *find_cmd(const char *cmd);
cmd_tbl_t *find_cmd_tbl (const char *cmd, cmd_tbl_t *table, int table_len);

extern int cmd_usage(cmd_tbl_t *cmdtp);

/*
 * Monitor Command
 *
 * All commands use a common argument format:
 *
 * void function (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
 */

typedef	void	command_t (cmd_tbl_t *, int, int, char *[]);

#define CMD_DATA_SIZE
extern int cmd_get_data_size(char* arg, int default_size);

/*
 * Command Flags:
 */
#define CMD_FLAG_REPEAT		0x0001	/* repeat last command		*/
#define CMD_FLAG_BOOTD		0x0002	/* command is from bootd	*/

#define Struct_Section  __attribute__ ((unused,section (".u_boot_cmd")))

#define U_BOOT_CMD(name,maxargs,rep,cmd,usage,help) \
cmd_tbl_t __u_boot_cmd_##name Struct_Section = {#name, maxargs, rep, cmd, usage, help}

#define U_BOOT_CMD_MKENT(name,maxargs,rep,cmd,usage,help) \
{#name, maxargs, rep, cmd, usage, help}

#endif	/* __COMMAND_H */
