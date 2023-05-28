#include <config.h>
#include <common.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdio_dev.h>
#include <serial.h>

DECLARE_GLOBAL_DATA_PTR;

/* stdio_dev 的链表头，仅做为链表头使用 */
static struct stdio_dev devs;
/* 分别指向 stdin  stdout  stderr 设备 */
struct stdio_dev *stdio_devices[] = { NULL, NULL, NULL };
char *stdio_names[MAX_FILES] = { "stdin", "stdout", "stderr" };

/**************************************************************************
 * SYSTEM DRIVERS
 **************************************************************************
 */

static void drv_system_init (void)
{
    /* 这里使用了局部变量 dev ，但在下面的 stdio_register 中将会分配空间并拷贝 */
	struct stdio_dev dev;

	memset (&dev, 0, sizeof (dev));

    /* 初始化设备的信息，名称，flag，对应的回调函数 
     * 这里的 dev 是对应到一个实际的设备的，具有对应的硬件操作函数 */
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
/* 获取 stdio_dev 设备的链表头 */
struct list_head* stdio_get_list(void)
{
	return &(devs.list);
}

/* 根据提供的名字查找具体的设备
 * 提供的名字就是 serial */
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

    /* _dev 是分配的内存，将传入的 dev 设备信息全部拷贝到 _dev 中 */
	memcpy(_dev, dev, sizeof(struct stdio_dev));
	strncpy(_dev->name, dev->name, 16);

	return _dev;
}

int stdio_register (struct stdio_dev * dev)
{
	struct stdio_dev *_dev;

    /* clone 一个 stdio_dev 设备，stdio_clone 函数中
     * 将会分配 stdio_dev 内存，并拷贝参数dev 对应的设备 */
	_dev = stdio_clone(dev);
	if(!_dev)
		return -1;

    /* 把设备 _dev 添加到 devs 链表尾部，以后会用到 */
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
