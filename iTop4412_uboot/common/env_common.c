#include <common.h>
#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>

DECLARE_GLOBAL_DATA_PTR;

#define DEBUGF(fmt,args...)

extern env_t *env_ptr;

extern void env_relocate_spec (void);
extern uchar env_get_char_spec(int);

static uchar env_get_char_init (int index);

/************************************************************************
 * Default settings to be used when no valid environment is found
 */
#define XMK_STR(x)	#x
#define MK_STR(x)	XMK_STR(x)

uchar default_environment[] = {
	"bootcmd="	CONFIG_BOOTCOMMAND		"\0"
	"bootdelay="	MK_STR(CONFIG_BOOTDELAY)	"\0"
	"baudrate="	MK_STR(CONFIG_BAUDRATE)		"\0"
	"\0"
};

int default_environment_size = sizeof(default_environment);

void env_crc_update (void)
{
	env_ptr->crc = crc32(0, env_ptr->data, ENV_SIZE);
}

static uchar env_get_char_init (int index)
{
	uchar c;

	/* if crc was bad, use the default environment */
	if (gd->env_valid)
	{
		c = env_get_char_spec(index);
	} else {
		c = default_environment[index];
	}

	return (c);
}

uchar env_get_char_memory (int index)
{
	if (gd->env_valid) {
		return ( *((uchar *)(gd->env_addr + index)) );
	} else {
		return ( default_environment[index] );
	}
}

uchar env_get_char (int index)
{
	uchar c;

	/* if relocated to RAM */
	if (gd->flags & GD_FLG_RELOC)
		c = env_get_char_memory(index);
	else
		c = env_get_char_init(index);

	return (c);
}

uchar *env_get_addr (int index)
{
	if (gd->env_valid) {
		return ( ((uchar *)(gd->env_addr + index)) );
	} else {
		return (&default_environment[index]);
	}
}

void set_default_env(void)
{
	if (sizeof(default_environment) > ENV_SIZE) {
		puts ("*** Error - default environment is too large\n\n");
		return;
	}

	memset(env_ptr, 0, sizeof(env_t));
	memcpy(env_ptr->data, default_environment,
	       sizeof(default_environment));
	env_crc_update ();
	gd->env_valid = 1;
}

void env_relocate (void)
{
	/*
	 * We must allocate a buffer for the environment
	 */
	env_ptr = (env_t *)malloc (CONFIG_ENV_SIZE);
	DEBUGF ("%s[%d] malloced ENV at %p\n", __FUNCTION__,__LINE__,env_ptr);

	if (gd->env_valid == 0) {
		puts ("*** Warning - bad CRC, using default environment\n\n");
		show_boot_progress (-60);
		set_default_env();
	}
	else {
		env_relocate_spec ();
	}
	gd->env_addr = (ulong)&(env_ptr->data);
}

