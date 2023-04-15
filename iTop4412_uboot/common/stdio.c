#include <config.h>
#include <common.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdio_dev.h>
#include <serial.h>

DECLARE_GLOBAL_DATA_PTR;

static struct stdio_dev devs;
struct stdio_dev *stdio_devices[] = { NULL, NULL, NULL };
char *stdio_names[MAX_FILES] = { "stdin", "stdout", "stderr" };

/**************************************************************************
 * SYSTEM DRIVERS
 **************************************************************************
 */

static void drv_system_init (void)
{
	struct stdio_dev dev;

	memset (&dev, 0, sizeof (dev));

	strcpy (dev.name, "serial");
	dev.flags = DEV_FLAGS_OUTPUT | DEV_FLAGS_INPUT | DEV_FLAGS_SYSTEM;
	dev.putc = serial_putc;
	dev.puts = serial_puts;
	dev.getc = serial_getc;
	dev.tstc = serial_tstc;

	stdio_register (&dev);
}

/**************************************************************************
 * DEVICES
 **************************************************************************
 */
struct list_head* stdio_get_list(void)
{
	return &(devs.list);
}

struct stdio_dev* stdio_get_by_name(char* name)
{
	struct list_head *pos;
	struct stdio_dev *dev;

	if(!name)
		return NULL;

	list_for_each(pos, &(devs.list)) {
		dev = list_entry(pos, struct stdio_dev, list);
		if(strcmp(dev->name, name) == 0)
			return dev;
	}

	return NULL;
}

struct stdio_dev* stdio_clone(struct stdio_dev *dev)
{
	struct stdio_dev *_dev;

	if(!dev)
		return NULL;

	_dev = calloc(1, sizeof(struct stdio_dev));

	if(!_dev)
		return NULL;

	memcpy(_dev, dev, sizeof(struct stdio_dev));
	strncpy(_dev->name, dev->name, 16);

	return _dev;
}

int stdio_register (struct stdio_dev * dev)
{
	struct stdio_dev *_dev;

	_dev = stdio_clone(dev);
	if(!_dev)
		return -1;
	list_add_tail(&(_dev->list), &(devs.list));
	return 0;
}

int stdio_init (void)
{
	/* Initialize the list */
	INIT_LIST_HEAD(&(devs.list));

	drv_system_init ();

	return (0);
}
