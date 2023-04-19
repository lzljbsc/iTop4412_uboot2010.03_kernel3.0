#ifndef _U_BOOT_ARM_H_
#define _U_BOOT_ARM_H_	1

/* for the following variables, see start.S */
extern ulong _armboot_start;	/* code start */
extern ulong _bss_start;	    /* code + data end == BSS start */
extern ulong _bss_end;		    /* BSS end */
extern ulong IRQ_STACK_START;	/* top of IRQ stack */
extern ulong FIQ_STACK_START;	/* top of FIQ stack */

/* cpu/.../cpu.c */
int	cpu_init(void);
int	cleanup_before_linux(void);

/* cpu/.../arch/cpu.c */
int	arch_cpu_init(void);
int	arch_misc_init(void);

/* board/.../... */
int	board_init(void);
int	dram_init (void);
void	setup_serial_tag (struct tag **params);
void	setup_revision_tag (struct tag **params);

/* ------------------------------------------------------------ */
/* Here is a list of some prototypes which are incompatible to	*/
/* the U-Boot implementation					*/
/* To be fixed!							*/
/* ------------------------------------------------------------ */
/* common/cmd_nvedit.c */
int	setenv (char *, char *);

/* cpu/.../interrupt.c */
int	arch_interrupt_init	(void);
void	reset_timer_masked	(void);
ulong	get_timer_masked	(void);
void	udelay_masked		(unsigned long usec);

/* cpu/.../timer.c */
int	timer_init		(void);

#endif	/* _U_BOOT_ARM_H_ */
