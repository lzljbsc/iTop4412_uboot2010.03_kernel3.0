/* linux/arch/arm/mach-exynos/include/mach/gpio.h
 *
 * Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS - gpio map definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_GPIO_H
#define __ASM_ARCH_GPIO_H __FILE__

#include "gpio-exynos4.h"
#include "gpio-exynos5.h"

//yulu
#if defined(CONFIG_ARCH_EXYNOS)
#define S3C_GPIO_SLP_OUT0       ((__force s3c_gpio_pull_t)0x00)
#define S3C_GPIO_SLP_OUT1       ((__force s3c_gpio_pull_t)0x01)
#define S3C_GPIO_SLP_INPUT      ((__force s3c_gpio_pull_t)0x02)
#define S3C_GPIO_SLP_PREV       ((__force s3c_gpio_pull_t)0x03)

#define S3C_GPIO_SETPIN_ZERO         0
#define S3C_GPIO_SETPIN_ONE          1
#define S3C_GPIO_SETPIN_NONE	     2
#endif

#define GPIO_LEVEL_LOW          0
#define GPIO_LEVEL_HIGH         1
#define GPIO_LEVEL_NONE         2
#define GPIO_INPUT              0
#define GPIO_OUTPUT             1

/* 配置文件中定义  CONFIG_ARCH_EXYNOS4 */
#if defined(CONFIG_ARCH_EXYNOS4)
/* S3C_GPIO_END 就是 GPIO总数量 */
#define S3C_GPIO_END		EXYNOS4_GPIO_END
/* EXYNOS4XXX_GPIO_END 与 EXYNOS5_GPIO_END 是相同的 
 * CONFIG_SAMSUNG_GPIO_EXTRA 定义为 0  */
#define ARCH_NR_GPIOS		(EXYNOS4XXX_GPIO_END +	\
				CONFIG_SAMSUNG_GPIO_EXTRA)
#elif defined(CONFIG_ARCH_EXYNOS5)
#define S3C_GPIO_END		EXYNOS5_GPIO_END
#define ARCH_NR_GPIOS		(EXYNOS5_GPIO_END +	\
				CONFIG_SAMSUNG_GPIO_EXTRA)
#else
#error "ARCH_EXYNOS* is not defined"
#endif

#include <asm-generic/gpio.h>

#endif /* __ASM_ARCH_GPIO_H */
