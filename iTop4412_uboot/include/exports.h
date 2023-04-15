#ifndef __EXPORTS_H__
#define __EXPORTS_H__

#include <common.h>

/* These are declarations of exported functions available in C code */
unsigned long get_version(void);
int  getc(void);
int  tstc(void);
void putc(const char);
void puts(const char*);
void printf(const char* fmt, ...);
void install_hdlr(int, interrupt_handler_t*, void*);
void free_hdlr(int);
void *malloc(size_t);
void free(void*);
void __udelay(unsigned long);
unsigned long get_timer(unsigned long);
void vprintf(const char *, va_list);
void do_reset (void);
unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base);
char *getenv (char *name);
int setenv (char *varname, char *varvalue);
long simple_strtol(const char *cp,char **endp,unsigned int base);
int strcmp(const char * cs,const char * ct);
int ustrtoul(const char *cp, char **endp, unsigned int base);
#include <spi.h>

void app_startup(char **);

enum {
#define EXPORT_FUNC(x) XF_ ## x ,
#include <_exports.h>
#undef EXPORT_FUNC

	XF_MAX
};

#define XF_VERSION	6

#endif	/* __EXPORTS_H__ */
