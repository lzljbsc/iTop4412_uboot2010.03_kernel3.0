#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_	1

/**************************************************************************
 *
 * The "environment" is stored as a list of '\0' terminated
 * "name=value" strings. The end of the list is marked by a double
 * '\0'. New entries are always added at the end. Deleting an entry
 * shifts the remaining entries to the front. Replacing an entry is a
 * combination of deleting the old value and adding the new one.
 *
 * The environment is preceeded by a 32 bit CRC over the data part.
 *
 **************************************************************************
 */

#include "compiler.h"

#define ENV_HEADER_SIZE	(sizeof(uint32_t))

#define ENV_SIZE (CONFIG_ENV_SIZE - ENV_HEADER_SIZE)

typedef	struct environment_s {
	uint32_t	crc;		/* CRC32 over data bytes	*/
	unsigned char	data[ENV_SIZE]; /* Environment data		*/
} env_t;

/* Function that returns a character from the environment */
unsigned char env_get_char (int);

/* Function that returns a pointer to a value from the environment */
unsigned char *env_get_addr(int);
unsigned char env_get_char_memory (int index);

/* Function that updates CRC of the enironment */
void env_crc_update (void);

/* [re]set to the default environment */
void set_default_env(void);

#endif	/* _ENVIRONMENT_H_ */
