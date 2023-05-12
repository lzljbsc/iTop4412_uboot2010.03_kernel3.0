#include <common.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdio_dev.h>
#include <exports.h>

DECLARE_GLOBAL_DATA_PTR;

static int console_setfile(int file, struct stdio_dev * dev)
{
	int error = 0;

	if (dev == NULL)
		return -1;

	switch (file) {
	case stdin:
	case stdout:
	case stderr:
		/* Start new device */
		if (dev->start) {
			error = dev->start();
			/* If it's not started dont use it */
			if (error < 0)
				break;
		}

		/* Assign the new device (leaving the existing one started) */
		stdio_devices[file] = dev;

		/*
		 * Update monitor functions
		 * (to use the console stuff by other applications)
		 */
		switch (file) {
		case stdin:
			gd->jt[XF_getc] = dev->getc;
			gd->jt[XF_tstc] = dev->tstc;
			break;
		case stdout:
			gd->jt[XF_putc] = dev->putc;
			gd->jt[XF_puts] = dev->puts;
			gd->jt[XF_printf] = printf;
			break;
		}
		break;

	default:		/* Invalid file ID */
		error = -1;
	}
	return error;
}

static inline int console_getc(int file)
{
	return stdio_devices[file]->getc();
}

static inline int console_tstc(int file)
{
	return stdio_devices[file]->tstc();
}

static inline void console_putc(int file, const char c)
{
	stdio_devices[file]->putc(c);
}

static inline void console_puts(int file, const char *s)
{
	stdio_devices[file]->puts(s);
}

static inline void console_printdevs(int file)
{
	printf("%s\n", stdio_devices[file]->name);
}

static inline void console_doenv(int file, struct stdio_dev *dev)
{
	console_setfile(file, dev);
}

/** U-Boot INITIAL CONSOLE-NOT COMPATIBLE FUNCTIONS *************************/

void serial_printf(const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[CONFIG_SYS_PBSIZE];

	va_start(args, fmt);

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf(printbuffer, fmt, args);
	va_end(args);

	serial_puts(printbuffer);
}

int fgetc(int file)
{
	if (file < MAX_FILES) {
		return console_getc(file);
	}

	return -1;
}

int ftstc(int file)
{
	if (file < MAX_FILES)
		return console_tstc(file);

	return -1;
}

void fputc(int file, const char c)
{
	if (file < MAX_FILES)
		console_putc(file, c);
}

void fputs(int file, const char *s)
{
	if (file < MAX_FILES)
		console_puts(file, s);
}

void fprintf(int file, const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[CONFIG_SYS_PBSIZE];

	va_start(args, fmt);

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf(printbuffer, fmt, args);
	va_end(args);

	/* Send to desired file */
	fputs(file, printbuffer);
}

/** U-Boot INITIAL CONSOLE-COMPATIBLE FUNCTION *****************************/

int getc(void)
{
	if (gd->flags & GD_FLG_DEVINIT) {
		/* Get from the standard input */
		return fgetc(stdin);
	}

	/* Send directly to the handler */
	return serial_getc();
}

int tstc(void)
{
	if (gd->flags & GD_FLG_DEVINIT) {
		/* Test the standard input */
		return ftstc(stdin);
	}

	/* Send directly to the handler */
	return serial_tstc();
}

void putc(const char c)
{
	if (gd->flags & GD_FLG_DEVINIT) {
		/* Send to the standard output */
		fputc(stdout, c);
	} else {
		/* Send directly to the handler */
		serial_putc(c);
	}
}

void puts(const char *s)
{
	if (gd->flags & GD_FLG_DEVINIT) {
		/* Send to the standard output */
		fputs(stdout, s);
	} else {
		/* Send directly to the handler */
		serial_puts(s);
	}
}

void printf(const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[CONFIG_SYS_PBSIZE];

	va_start(args, fmt);

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf(printbuffer, fmt, args);
	va_end(args);

	/* Print the string */
	puts(printbuffer);
}

void vprintf(const char *fmt, va_list args)
{
	uint i;
	char printbuffer[CONFIG_SYS_PBSIZE];

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsprintf(printbuffer, fmt, args);

	/* Print the string */
	puts(printbuffer);
}

/* test if ctrl-c was pressed */
static int ctrlc_disabled = 0;	/* see disable_ctrl() */
static int ctrlc_was_pressed = 0;
int ctrlc(void)
{
	if (!ctrlc_disabled && gd->have_console) {
		if (tstc()) {
			switch (getc()) {
			case 0x03:		/* ^C - Control C */
				ctrlc_was_pressed = 1;
				return 1;
			default:
				break;
			}
		}
	}
	return 0;
}

/* pass 1 to disable ctrlc() checking, 0 to enable.
 * returns previous state
 */
int disable_ctrlc(int disable)
{
	int prev = ctrlc_disabled;	/* save previous state */

	ctrlc_disabled = disable;
	return prev;
}

int had_ctrlc (void)
{
	return ctrlc_was_pressed;
}

void clear_ctrlc(void)
{
	ctrlc_was_pressed = 0;
}

inline void dbg(const char *fmt, ...)
{
}

/** U-Boot INIT FUNCTIONS *************************************************/

struct stdio_dev *search_device(int flags, char *name)
{
	struct stdio_dev *dev;

	dev = stdio_get_by_name(name);

	if (dev && (dev->flags & flags))
		return dev;

	return NULL;
}

int console_assign(int file, char *devname)
{
	int flag;
	struct stdio_dev *dev;

	/* Check for valid file */
	switch (file) {
	case stdin:
		flag = DEV_FLAGS_INPUT;
		break;
	case stdout:
	case stderr:
		flag = DEV_FLAGS_OUTPUT;
		break;
	default:
		return -1;
	}

	/* Check for valid device name */

	dev = search_device(flag, devname);

	if (dev)
		return console_setfile(file, dev);

	return -1;
}

/* Called before relocation - use serial functions */
int console_init_f(void)
{
    /* 设置为有控制台 */
	gd->have_console = 1;

	return 0;
}

void stdio_print_current_devices(void)
{
	/* Print information */
	puts("In:    ");
	if (stdio_devices[stdin] == NULL) {
		puts("No input devices available!\n");
	} else {
		printf ("%s\n", stdio_devices[stdin]->name);
	}

	puts("Out:   ");
	if (stdio_devices[stdout] == NULL) {
		puts("No output devices available!\n");
	} else {
		printf ("%s\n", stdio_devices[stdout]->name);
	}

	puts("Err:   ");
	if (stdio_devices[stderr] == NULL) {
		puts("No error devices available!\n");
	} else {
		printf ("%s\n", stdio_devices[stderr]->name);
	}
}

/* Called after the relocation - use desired console functions */
int console_init_r(void)
{
	struct stdio_dev *inputdev = NULL, *outputdev = NULL;
	int i;
	struct list_head *list = stdio_get_list();
	struct list_head *pos;
	struct stdio_dev *dev;

	/* Scan devices looking for input and output devices */
	list_for_each(pos, list) {
		dev = list_entry(pos, struct stdio_dev, list);

		if ((dev->flags & DEV_FLAGS_INPUT) && (inputdev == NULL)) {
			inputdev = dev;
		}
		if ((dev->flags & DEV_FLAGS_OUTPUT) && (outputdev == NULL)) {
			outputdev = dev;
		}
		if(inputdev && outputdev)
			break;
	}

	/* Initializes output console first */
	if (outputdev != NULL) {
		console_setfile(stdout, outputdev);
		console_setfile(stderr, outputdev);
	}

	/* Initializes input console */
	if (inputdev != NULL) {
		console_setfile(stdin, inputdev);
	}

	gd->flags |= GD_FLG_DEVINIT;	/* device initialization completed */

	stdio_print_current_devices();

	/* Setting environment variables */
	for (i = 0; i < 3; i++) {
		setenv(stdio_names[i], stdio_devices[i]->name);
	}

	return 0;
}

