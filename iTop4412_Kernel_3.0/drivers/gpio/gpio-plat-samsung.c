/* arch/arm/plat-samsung/gpiolib.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * SAMSUNG - GPIOlib support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <plat/gpio-core.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-cfg-helpers.h>

#if defined(CONFIG_MTK_COMBO_COMM) || defined(CONFIG_MTK_COMBO_COMM_MODULE)	//add by cym 20130301
#include <linux/module.h>
#endif

#ifndef DEBUG_GPIO
#define gpio_dbg(x...) do { } while (0)
#else
#define gpio_dbg(x...) printk(KERN_DEBUG x)
#endif

/* The samsung_gpiolib_4bit routines are to control the gpio banks where
 * the gpio configuration register (GPxCON) has 4 bits per GPIO, as the
 * following example:
 *
 * base + 0x00: Control register, 4 bits per gpio
 *		gpio n: 4 bits starting at (4*n)
 *		0000 = input, 0001 = output, others mean special-function
 * base + 0x04: Data register, 1 bit per gpio
 *		bit n: data bit n
 *
 * Note, since the data register is one bit per gpio and is at base + 0x4
 * we can use s3c_gpiolib_get and s3c_gpiolib_set to change the state of
 * the output.
*/
/* 配置gpio口为输入
 * 传入的参数是 gpiolib 中的结构 */
static int samsung_gpiolib_4bit_input(struct gpio_chip *chip,
				      unsigned int offset)
{
    /* 利用 container_of 大法找到平台相关结构 */
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
    /* 对应GPIO组的寄存器基地址 */
	void __iomem *base = ourchip->base;
	unsigned long flags;
	unsigned long con;

	s3c_gpio_lock(ourchip, flags);

    /* 先获取原配置，这里只能获取整组GPIO的配置，不能单独某个GPIO口 */
	con = __raw_readl(base + GPIOCON_OFF);
    /* 将GPIO对应的寄存器位清零，输入模式配置就是 0x0，所以不需要配其他的了 */
	con &= ~(0xf << con_4bit_shift(offset));
    /* 配置写会寄存器，配置完成了 */
	__raw_writel(con, base + GPIOCON_OFF);

	s3c_gpio_unlock(ourchip, flags);

	gpio_dbg("%s: %p: CON now %08lx\n", __func__, base, con);

	return 0;
}

#if defined(CONFIG_MTK_COMBO_COMM) || defined(CONFIG_MTK_COMBO_COMM_MODULE)	//add by cym 20130301
int samsung_gpiolib_4bit_output(struct gpio_chip *chip,
				       unsigned int offset, int value)
#else
static int samsung_gpiolib_4bit_output(struct gpio_chip *chip,
				       unsigned int offset, int value)
#endif
{
    /* 利用 container_of 大法找到平台相关结构 */
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
    /* 对应GPIO组的寄存器基地址 */
	void __iomem *base = ourchip->base;
	unsigned long flags;
	unsigned long con;
	unsigned long dat;

	s3c_gpio_lock(ourchip, flags);

    /* 先获取原配置，这里只能获取整组GPIO的配置，不能单独某个GPIO口 */
	con = __raw_readl(base + GPIOCON_OFF);
    /* 先清除原配置（清0），在写入新的配置（0x1，对应 输出） */
	con &= ~(0xf << con_4bit_shift(offset));
	con |= 0x1 << con_4bit_shift(offset);

    /* 获取当前GPIO组的数据寄存器值，该寄存器每个位对应一个GPIO口
     * 输入模式下对应GPIO端口状态
     * 输出模式下对应输出状态 */
	dat = __raw_readl(base + GPIODAT_OFF);

    /* 默认输出电平 */
	if (value)
		dat |= 1 << offset;
	else
		dat &= ~(1 << offset);

    /* 将配置寄存器值写入寄存器，包含了输出模式和默认输出电平 */
	__raw_writel(dat, base + GPIODAT_OFF);
	__raw_writel(con, base + GPIOCON_OFF);
	__raw_writel(dat, base + GPIODAT_OFF);

	s3c_gpio_unlock(ourchip, flags);

	gpio_dbg("%s: %p: CON %08lx, DAT %08lx\n", __func__, base, con, dat);

	return 0;
}

#if defined(CONFIG_MTK_COMBO_COMM) || defined(CONFIG_MTK_COMBO_COMM_MODULE)	//add by cym 20130301
EXPORT_SYMBOL(samsung_gpiolib_4bit_output);
#endif

/* The next set of routines are for the case where the GPIO configuration
 * registers are 4 bits per GPIO but there is more than one register (the
 * bank has more than 8 GPIOs.
 *
 * This case is the similar to the 4 bit case, but the registers are as
 * follows:
 *
 * base + 0x00: Control register, 4 bits per gpio (lower 8 GPIOs)
 *		gpio n: 4 bits starting at (4*n)
 *		0000 = input, 0001 = output, others mean special-function
 * base + 0x04: Control register, 4 bits per gpio (up to 8 additions GPIOs)
 *		gpio n: 4 bits starting at (4*n)
 *		0000 = input, 0001 = output, others mean special-function
 * base + 0x08: Data register, 1 bit per gpio
 *		bit n: data bit n
 *
 * To allow us to use the s3c_gpiolib_get and s3c_gpiolib_set routines we
 * store the 'base + 0x4' address so that these routines see the data
 * register at ourchip->base + 0x04.
 */

static int samsung_gpiolib_4bit2_input(struct gpio_chip *chip,
				       unsigned int offset)
{
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
	void __iomem *base = ourchip->base;
	void __iomem *regcon = base;
	unsigned long flags;
	unsigned long con;

	if (offset > 7)
		offset -= 8;
	else
		regcon -= 4;

	s3c_gpio_lock(ourchip, flags);

	con = __raw_readl(regcon);
	con &= ~(0xf << con_4bit_shift(offset));
	__raw_writel(con, regcon);

	s3c_gpio_unlock(ourchip, flags);

	gpio_dbg("%s: %p: CON %08lx\n", __func__, base, con);

	return 0;
}

static int samsung_gpiolib_4bit2_output(struct gpio_chip *chip,
					unsigned int offset, int value)
{
	struct s3c_gpio_chip *ourchip = to_s3c_gpio(chip);
	void __iomem *base = ourchip->base;
	void __iomem *regcon = base;
	unsigned long flags;
	unsigned long con;
	unsigned long dat;
	unsigned con_offset = offset;

	if (con_offset > 7)
		con_offset -= 8;
	else
		regcon -= 4;

	s3c_gpio_lock(ourchip, flags);

	con = __raw_readl(regcon);
	con &= ~(0xf << con_4bit_shift(con_offset));
	con |= 0x1 << con_4bit_shift(con_offset);

	dat = __raw_readl(base + GPIODAT_OFF);

	if (value)
		dat |= 1 << offset;
	else
		dat &= ~(1 << offset);

	__raw_writel(dat, base + GPIODAT_OFF);
	__raw_writel(con, regcon);
	__raw_writel(dat, base + GPIODAT_OFF);

	s3c_gpio_unlock(ourchip, flags);

	gpio_dbg("%s: %p: CON %08lx, DAT %08lx\n", __func__, base, con, dat);

	return 0;
}

void __init samsung_gpiolib_add_4bit(struct s3c_gpio_chip *chip)
{
    /* 设置 gpiolib 中的结构体 gpio_chip 的成员，
     * 在调用内核提供的gpio控制函数时，最终会调用到这里 */
	chip->chip.direction_input = samsung_gpiolib_4bit_input;
	chip->chip.direction_output = samsung_gpiolib_4bit_output;
    /* 电源管理部分，不属于 gpiolib 部分 */
	chip->pm = __gpio_pm(&s3c_gpio_pm_4bit);
}

void __init samsung_gpiolib_add_4bit2(struct s3c_gpio_chip *chip)
{
	chip->chip.direction_input = samsung_gpiolib_4bit2_input;
	chip->chip.direction_output = samsung_gpiolib_4bit2_output;
	chip->pm = __gpio_pm(&s3c_gpio_pm_4bit);
}

/* 逐个 s3c_gpio_chip 注册到 gpiolib */
void __init samsung_gpiolib_add_4bit_chips(struct s3c_gpio_chip *chip,
					   int nr_chips)
{
	for (; nr_chips > 0; nr_chips--, chip++) {
		samsung_gpiolib_add_4bit(chip);
		s3c_gpiolib_add(chip);
	}
}

/* 这个函数在 4412中并未使用
 * 该函数适用于每个 GPIO口需要 4bit的配置，
 * 但每组GPIO多于8个，也就是控制寄存器或其他寄存器有两个 寄存器的情况 */
void __init samsung_gpiolib_add_4bit2_chips(struct s3c_gpio_chip *chip,
					    int nr_chips)
{
	for (; nr_chips > 0; nr_chips--, chip++) {
		samsung_gpiolib_add_4bit2(chip);
		s3c_gpiolib_add(chip);
	}
}

/* 这个函数在 4412中并未使用
 * 该函数适用于每个 GPIO口需要 2bit的配置， */
void __init samsung_gpiolib_add_2bit_chips(struct s3c_gpio_chip *chip,
					   int nr_chips)
{
	for (; nr_chips > 0; nr_chips--, chip++)
		s3c_gpiolib_add(chip);
}
