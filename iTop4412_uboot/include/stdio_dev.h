#ifndef _STDIO_DEV_H_
#define _STDIO_DEV_H_

#include <linux/list.h>

/*
 * STDIO DEVICES
 */

#define DEV_FLAGS_INPUT	 0x00000001	/* Device can be used as input	console */
#define DEV_FLAGS_OUTPUT 0x00000002	/* Device can be used as output console */
#define DEV_FLAGS_SYSTEM 0x80000000	/* Device is a system device		*/
#define DEV_EXT_VIDEO	 0x00000001	/* Video extensions supported		*/

/* Device information */
struct stdio_dev {
	int	flags;			/* Device flags: input/output/system	*/
	int	ext;			/* Supported extensions			*/
	char	name[16];		/* Device name				*/

/* GENERAL functions */

	int (*start) (void);		/* To start the device			*/
	int (*stop) (void);		/* To stop the device			*/

/* OUTPUT functions */

	void (*putc) (const char c);	/* To put a char			*/
	void (*puts) (const char *s);	/* To put a string (accelerator)	*/

/* INPUT functions */

	int (*tstc) (void);		/* To test if a char is ready...	*/
	int (*getc) (void);		/* To get that char			*/

/* Other functions */

	void *priv;			/* Private extensions			*/
	struct list_head list;
};

/*
 * VIDEO EXTENSIONS
 */
#define VIDEO_FORMAT_RGB_INDEXED	0x0000
#define VIDEO_FORMAT_RGB_DIRECTCOLOR	0x0001
#define VIDEO_FORMAT_YUYV_4_4_4		0x0010
#define VIDEO_FORMAT_YUYV_4_2_2		0x0011

typedef struct {
	void *address;			/* Address of framebuffer		*/
	ushort	width;			/* Horizontal resolution		*/
	ushort	height;			/* Vertical resolution			*/
	uchar	format;			/* Format				*/
	uchar	colors;			/* Colors number or color depth		*/
	void (*setcolreg) (int, int, int, int);
	void (*getcolreg) (int, void *);
} video_ext_t;

/*
 * VARIABLES
 */
extern struct stdio_dev *stdio_devices[];
extern char *stdio_names[MAX_FILES];

/*
 * PROTOTYPES
 */
int	stdio_register (struct stdio_dev * dev);
int	stdio_init (void);
void	stdio_print_current_devices(void);
struct list_head* stdio_get_list(void);
struct stdio_dev* stdio_get_by_name(char* name);
struct stdio_dev* stdio_clone(struct stdio_dev *dev);

#endif
