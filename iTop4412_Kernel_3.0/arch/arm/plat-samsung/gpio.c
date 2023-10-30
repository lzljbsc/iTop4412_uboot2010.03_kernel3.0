/* linux/arch/arm/plat-s3c/gpio.c
 *
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C series GPIO core
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#if defined(CONFIG_MTK_COMBO_COMM) || defined(CONFIG_MTK_COMBO_COMM_MODULE)	//add by cym 20130301
#include <linux/module.h>
#endif

#include <plat/gpio-core.h>

#ifdef CONFIG_S3C_GPIO_TRACK
/* 该结构体数组包含了所有GPIO的 s3c_gpio_chip 
 * 在其他的一些函数中会使用到，用于查找回调函数 
 * 如 s3c_gpio_cfgpin 等复用功能配置函数
 * 这个作用与 gpiolib 中的 gpio_chip 结构类似，
 * 只使用 gpio_chip 结构也是可以反向查找的 */
struct s3c_gpio_chip *s3c_gpios[S3C_GPIO_END];

/* 将gpio 的chip 结构添加到 s3c_gpios 数组中 */
static __init void s3c_gpiolib_track(struct s3c_gpio_chip *chip)
{
	unsigned int gpn;
	int i;

	gpn = chip->chip.base;
	for (i = 0; i < chip->chip.ngpio; i++, gpn++) {
		BUG_ON(gpn >= ARRAY_SIZE(s3c_gpios));
		s3c_gpios[gpn] = chip;
	}
}
#endif /* CONFIG_S3C_GPIO_TRACK */

/* Default routines for controlling GPIO, based on the original S3C24XX
 * GPIO functions which deal with the case where each gpio bank of the
 * chip is as following:
 *
 * base + 0x00: Control register, 2 bits per gpio
 *	        gpio n: 2 bits starting at (2*n)
 *		00 = input, 01 = output, others mean special-function
 * base + 0x04: Data register, 1 bit per gpio
 *		bit n: data bit n
*/

/* 该函数对应的是每个GPIO口有 2bit配置的情况，4412不适用 */
static int s3c_gpiolib_input(struct gpio_chip *chip, unsigned offset)
{
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
	void __iomem *base = ourchip->base;
	unsigned long flags;
	unsigned long con;

	s3c_gpio_lock(ourchip, flags);

	con = __raw_readl(base + 0x00);
	con &= ~(3 << (offset * 2));

	__raw_writel(con, base + 0x00);

	s3c_gpio_unlock(ourchip, flags);
	return 0;
}

/* 该函数对应的是每个GPIO口有 2bit配置的情况，4412不适用 */
static int s3c_gpiolib_output(struct gpio_chip *chip,
			      unsigned offset, int value)
{
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
	void __iomem *base = ourchip->base;
	unsigned long flags;
	unsigned long dat;
	unsigned long con;

	s3c_gpio_lock(ourchip, flags);

	dat = __raw_readl(base + 0x04);
	dat &= ~(1 << offset);
	if (value)
		dat |= 1 << offset;
	__raw_writel(dat, base + 0x04);

	con = __raw_readl(base + 0x00);
	con &= ~(3 << (offset * 2));
	con |= 1 << (offset * 2);

	__raw_writel(con, base + 0x00);
	__raw_writel(dat, base + 0x04);

	s3c_gpio_unlock(ourchip, flags);
	return 0;
}

/* 设置GPIO输出值 */
static void s3c_gpiolib_set(struct gpio_chip *chip,
			    unsigned offset, int value)
{
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
	void __iomem *base = ourchip->base;
	unsigned long flags;
	unsigned long dat;

	s3c_gpio_lock(ourchip, flags);

    /* 读出原数据寄存器值，并设置相应位，再写回寄存器 */
	dat = __raw_readl(base + 0x04);
	dat &= ~(1 << offset);
	if (value)
		dat |= 1 << offset;
	__raw_writel(dat, base + 0x04);

	s3c_gpio_unlock(ourchip, flags);
}

/* 获取GPIO输入值 */
#if defined(CONFIG_MTK_COMBO_COMM) || defined(CONFIG_MTK_COMBO_COMM_MODULE)	//add by cym 20130301
int s3c_gpiolib_get(struct gpio_chip *chip, unsigned offset)
#else
static int s3c_gpiolib_get(struct gpio_chip *chip, unsigned offset)
#endif
{
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
	unsigned long val;

    /* 读出数据寄存器值，并返回状态值 */
	val = __raw_readl(ourchip->base + 0x04);
	val >>= offset;
	val &= 1;

	return val;
}
#if defined(CONFIG_MTK_COMBO_COMM) || defined(CONFIG_MTK_COMBO_COMM_MODULE)	//add by cym 20130301
EXPORT_SYMBOL(s3c_gpiolib_get);
#endif

/* 根据传入的 s3c_gpio_chip 结构体，完善其内部的 gpio_chip 结构，
 * 并向 gpiolib 中注册
 * 典型的操作方法， gpiolib 是内核提供的，包含了基本的gpio操作，
 * 而 s3c_gpio_chip 结构是 三星根据gpio特点定义的，内部包含有gpio_chip 结构 
 * 并且还包含有其他的一些成员，设置复用功能，上下拉，寄存器地址等信息 
 * 并且可以通过 gpio_chip 结构反向找到 s3c_gpio_chip 结构，完成具体的操作 */
__init void s3c_gpiolib_add(struct s3c_gpio_chip *chip)
{
	struct gpio_chip *gc = &chip->chip;
	int ret;

    /* 检查基本的配置量 */
	BUG_ON(!chip->base);
	BUG_ON(!gc->label);
	BUG_ON(!gc->ngpio);

	spin_lock_init(&chip->lock);

    /* direction_input 和 direction_output 
     * 是否已设置需要根据前面的初始化确认
     * 这里提供的 s3c_gpiolib_input 和 s3c_gpiolib_output 
     * 是每个GPIO口有 2bit 配置寄存器的函数 
     * 在 4412 芯片中，所有GPIO都是 4bit配置的 */
	if (!gc->direction_input)
		gc->direction_input = s3c_gpiolib_input;
	if (!gc->direction_output)
		gc->direction_output = s3c_gpiolib_output;
    /* set 和 get 前面都没有初始化 */
	if (!gc->set)
		gc->set = s3c_gpiolib_set;
	if (!gc->get)
		gc->get = s3c_gpiolib_get;

    /* .config 中有该配置项 */
#ifdef CONFIG_PM
    /* 这里在前面的初始化时已经赋值了 */
	if (chip->pm != NULL) {
		if (!chip->pm->save || !chip->pm->resume)
			printk(KERN_ERR "gpio: %s has missing PM functions\n",
			       gc->label);
	} else
		printk(KERN_ERR "gpio: %s has no PM function\n", gc->label);
#endif

    /* 将这一组的 chip 添加到 gpiolib 中 */
	/* gpiochip_add() prints own failure message on error. */
	ret = gpiochip_add(gc);
    if (ret >= 0) {
        /* s3c_gpiolib_track 函数
         * 将 s3c_gpio_chip 结构添加到全局结构体数组 s3c_chip 中
         * 这个数组在 s3c_gpio_cfgpin 等函数中会用到 */
		s3c_gpiolib_track(chip);
    }
}

// TODO: 关于 to_irq 的分析
int samsung_gpiolib_to_irq(struct gpio_chip *chip, unsigned int offset)
{
	struct s3c_gpio_chip *s3c_chip = container_of(chip,
			struct s3c_gpio_chip, chip);

	return s3c_chip->irq_base + offset;
}
