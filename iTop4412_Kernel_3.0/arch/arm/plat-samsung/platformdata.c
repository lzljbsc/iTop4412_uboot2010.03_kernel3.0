/* linux/arch/arm/plat-samsung/platformdata.c
 *
 * Copyright 2010 Ben Dooks <ben-linux <at> fluff.org>
 *
 * Helper for platform data setting
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>

#include <plat/devs.h>

void __init *s3c_set_platdata(void *pd, size_t pdsize,
			      struct platform_device *pdev)
{
	void *npd;

	if (!pd) {
		/* too early to use dev_name(), may not be registered */
		printk(KERN_ERR "%s: no platform data supplied\n", pdev->name);
		return NULL;
	}

    /* kmemdup 是重新分配一段内存，并拷贝一份
     * 重新拷贝一份，是因为 pd 指向的原始数据，可能会被多个 设备使用
     * 如果不重新拷贝一份，更改 pd的内容，会导致错误 */
	npd = kmemdup(pd, pdsize, GFP_KERNEL);
	if (!npd) {
		printk(KERN_ERR "%s: cannot clone platform data\n", pdev->name);
		return NULL;
	}

    /* 新的 npd 做为 platform_device 的数据 */
	pdev->dev.platform_data = npd;
	return npd;
}
