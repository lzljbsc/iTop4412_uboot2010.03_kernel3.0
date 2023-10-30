/* linux/arch/arm/plat-s3c/gpio-config.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008-2010 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C series GPIO configuration core
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/io.h>

#include <plat/gpio-core.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-cfg-helpers.h>

/* 底层的GPIO复用功能配置函数
 * 传入的是GPIO编号，如 EXYNOS4_GPX0(1), 
 * 复用功能传入的是 S3C_GPIO_SFN(2) 
 * 每个GPIO对应的复用功能，需要参考手册
 * 这个函数中，根据传入的GPIO编号查找 gpio_chip 结构，
 * 再找到对应的 s3c_gpio_chip 结构，
 * 然后调用 s3c_gpio_cfg 结构中的回调函数 */
int s3c_gpio_cfgpin(unsigned int pin, unsigned int config)
{
    /* 查找到 s3c_gpio_chip 结构，包含有回调函数 */
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	unsigned long flags;
	int offset;
	int ret;

	if (!chip)
		return -EINVAL;

	offset = pin - chip->chip.base;

	s3c_gpio_lock(chip, flags);
	ret = s3c_gpio_do_setcfg(chip, offset, config);
	s3c_gpio_unlock(chip, flags);

	return ret;
}
EXPORT_SYMBOL(s3c_gpio_cfgpin);

/* 配置一组GPIO 功能 */
int s3c_gpio_cfgpin_range(unsigned int start, unsigned int nr,
			  unsigned int cfg)
{
	int ret;

	for (; nr > 0; nr--, start++) {
		ret = s3c_gpio_cfgpin(start, cfg);
		if (ret != 0)
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(s3c_gpio_cfgpin_range);

/* 带有上下拉配置的 一组GPIO配置 */
int s3c_gpio_cfgall_range(unsigned int start, unsigned int nr,
			  unsigned int cfg, s3c_gpio_pull_t pull)
{
	int ret;

	for (; nr > 0; nr--, start++) {
		s3c_gpio_setpull(start, pull);
		ret = s3c_gpio_cfgpin(start, cfg);
		if (ret != 0)
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(s3c_gpio_cfgall_range);

/* 获取当前GPIO配置 */
unsigned s3c_gpio_getcfg(unsigned int pin)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	unsigned long flags;
	unsigned ret = 0;
	int offset;

	if (chip) {
		offset = pin - chip->chip.base;

		s3c_gpio_lock(chip, flags);
		ret = s3c_gpio_do_getcfg(chip, offset);
		s3c_gpio_unlock(chip, flags);
	}

	return ret;
}
EXPORT_SYMBOL(s3c_gpio_getcfg);


/* 设置GPIO上下拉 */
int s3c_gpio_setpull(unsigned int pin, s3c_gpio_pull_t pull)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	unsigned long flags;
	int offset, ret;

	if (!chip)
		return -EINVAL;

	offset = pin - chip->chip.base;

	s3c_gpio_lock(chip, flags);
	ret = s3c_gpio_do_setpull(chip, offset, pull);
	s3c_gpio_unlock(chip, flags);

	return ret;
}
EXPORT_SYMBOL(s3c_gpio_setpull);

/* 获取GPIO上下拉 */
s3c_gpio_pull_t s3c_gpio_getpull(unsigned int pin)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	unsigned long flags;
	int offset;
	u32 pup = 0;

	if (chip) {
		offset = pin - chip->chip.base;

		s3c_gpio_lock(chip, flags);
		pup = s3c_gpio_do_getpull(chip, offset);
		s3c_gpio_unlock(chip, flags);
	}

	return (__force s3c_gpio_pull_t)pup;
}
EXPORT_SYMBOL(s3c_gpio_getpull);

/* 下面的函数 4412 中并未使用 */
#ifdef CONFIG_S3C_GPIO_CFG_S3C24XX
int s3c_gpio_setcfg_s3c24xx_a(struct s3c_gpio_chip *chip,
			      unsigned int off, unsigned int cfg)
{
	void __iomem *reg = chip->base;
	unsigned int shift = off;
	u32 con;

	if (s3c_gpio_is_cfg_special(cfg)) {
		cfg &= 0xf;

		/* Map output to 0, and SFN2 to 1 */
		cfg -= 1;
		if (cfg > 1)
			return -EINVAL;

		cfg <<= shift;
	}

	con = __raw_readl(reg);
	con &= ~(0x1 << shift);
	con |= cfg;
	__raw_writel(con, reg);

	return 0;
}

unsigned s3c_gpio_getcfg_s3c24xx_a(struct s3c_gpio_chip *chip,
				   unsigned int off)
{
	u32 con;

	con = __raw_readl(chip->base);
	con >>= off;
	con &= 1;
	con++;

	return S3C_GPIO_SFN(con);
}

int s3c_gpio_setcfg_s3c24xx(struct s3c_gpio_chip *chip,
			    unsigned int off, unsigned int cfg)
{
	void __iomem *reg = chip->base;
	unsigned int shift = off * 2;
	u32 con;

	if (s3c_gpio_is_cfg_special(cfg)) {
		cfg &= 0xf;
		if (cfg > 3)
			return -EINVAL;

		cfg <<= shift;
	}

	con = __raw_readl(reg);
	con &= ~(0x3 << shift);
	con |= cfg;
	__raw_writel(con, reg);

	return 0;
}

unsigned int s3c_gpio_getcfg_s3c24xx(struct s3c_gpio_chip *chip,
				     unsigned int off)
{
	u32 con;

	con = __raw_readl(chip->base);
	con >>= off * 2;
	con &= 3;

	/* this conversion works for IN and OUT as well as special mode */
	return S3C_GPIO_SPECIAL(con);
}
#endif

#ifdef CONFIG_S3C_GPIO_CFG_S3C64XX
/* 底层配置接口
 * 用于根据传入的GPIO复用功能配置项，配置对应的GPIO口
 * 函数实现主要是根据 off 偏移计算GPIO配置寄存器中需要配置的数值
 * 再写入对应寄存器即可 */
int s3c_gpio_setcfg_s3c64xx_4bit(struct s3c_gpio_chip *chip,
				 unsigned int off, unsigned int cfg)
{
	void __iomem *reg = chip->base;
	unsigned int shift = (off & 7) * 4;
	u32 con;

	if (off < 8 && chip->chip.ngpio > 8)
		reg -= 4;

	if (s3c_gpio_is_cfg_special(cfg)) {
		cfg &= 0xf;
		cfg <<= shift;
	}

	con = __raw_readl(reg);
	con &= ~(0xf << shift);
	con |= cfg;
	__raw_writel(con, reg);

	return 0;
}

/* 底层获取配置
 * 上面函数的逆过程，实现也比较简单 */
unsigned s3c_gpio_getcfg_s3c64xx_4bit(struct s3c_gpio_chip *chip,
				      unsigned int off)
{
	void __iomem *reg = chip->base;
	unsigned int shift = (off & 7) * 4;
	u32 con;

	if (off < 8 && chip->chip.ngpio > 8)
		reg -= 4;

	con = __raw_readl(reg);
	con >>= shift;
	con &= 0xf;

	/* this conversion works for IN and OUT as well as special mode */
	return S3C_GPIO_SPECIAL(con);
}

#endif /* CONFIG_S3C_GPIO_CFG_S3C64XX */

#ifdef CONFIG_S3C_GPIO_PULL_UPDOWN
/* 底层的上下拉操作函数，直接使用虚拟地址操作寄存器 */
int s3c_gpio_setpull_updown(struct s3c_gpio_chip *chip,
			    unsigned int off, s3c_gpio_pull_t pull)
{
	void __iomem *reg = chip->base + 0x08;
	int shift = off * 2;
	u32 pup;

	pup = __raw_readl(reg);
	pup &= ~(3 << shift);
	pup |= pull << shift;
	__raw_writel(pup, reg);

	return 0;
}

/* 底层的上下拉操作函数，直接使用虚拟地址操作寄存器 */
s3c_gpio_pull_t s3c_gpio_getpull_updown(struct s3c_gpio_chip *chip,
					unsigned int off)
{
	void __iomem *reg = chip->base + 0x08;
	int shift = off * 2;
	u32 pup = __raw_readl(reg);

	pup >>= shift;
	pup &= 0x3;
	return (__force s3c_gpio_pull_t)pup;
}
#endif

/* GPIO 驱动能力配置 */
#ifdef CONFIG_S5P_GPIO_DRVSTR
s5p_gpio_drvstr_t s5p_gpio_get_drvstr(unsigned int pin)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	unsigned int off;
	void __iomem *reg;
	int shift;
	u32 drvstr;

	if (!chip)
		return -EINVAL;

	off = pin - chip->chip.base;
	shift = off * 2;
	reg = chip->base + 0x0C;

	drvstr = __raw_readl(reg);
	drvstr = drvstr >> shift;
	drvstr &= 0x3;

	return (__force s5p_gpio_drvstr_t)drvstr;
}
EXPORT_SYMBOL(s5p_gpio_get_drvstr);

/* 设置GPIO驱动能力 */
int s5p_gpio_set_drvstr(unsigned int pin, s5p_gpio_drvstr_t drvstr)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	unsigned int off;
	void __iomem *reg;
	int shift;
	u32 tmp;

	if (!chip)
		return -EINVAL;

	off = pin - chip->chip.base;
	shift = off * 2;
	reg = chip->base + 0x0C;

	tmp = __raw_readl(reg);
	tmp &= ~(0x3 << shift);
	tmp |= drvstr << shift;

	__raw_writel(tmp, reg);

	return 0;
}
EXPORT_SYMBOL(s5p_gpio_set_drvstr);
#endif	/* CONFIG_S5P_GPIO_DRVSTR */

/* 掉电模式下GPIO状态相关处理
 * 该代码中并未使用，也就是都用的芯片模式值 
 * 可以根据单板进行配置 */
s5p_gpio_pd_cfg_t s5p_gpio_get_pd_cfg(unsigned int pin)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	unsigned int off;
	void __iomem *reg;
	int shift;
	u32 pd_cfg;

	if (!chip)
		return -EINVAL;

	off = pin - chip->chip.base;
	shift = off * 2;
	reg = chip->base + 0x10;

	pd_cfg = __raw_readl(reg);
	pd_cfg = pd_cfg >> shift;
	pd_cfg &= 0x3;

	return (__force s5p_gpio_pd_cfg_t)pd_cfg;
}
EXPORT_SYMBOL(s5p_gpio_get_pd_cfg);

int s5p_gpio_set_pd_cfg(unsigned int pin, s5p_gpio_pd_cfg_t pd_cfg)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	unsigned int off;
	void __iomem *reg;
	int shift;
	u32 tmp;

	if (!chip)
		return -EINVAL;

	off = pin - chip->chip.base;
	shift = off * 2;
	reg = chip->base + 0x10;

	tmp = __raw_readl(reg);
	tmp &= ~(0x3 << shift);
	tmp |= pd_cfg << shift;

	__raw_writel(tmp, reg);

	return 0;
}
EXPORT_SYMBOL(s5p_gpio_set_pd_cfg);

s5p_gpio_pd_pull_t s5p_gpio_get_pd_pull(unsigned int pin)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	unsigned int off;
	void __iomem *reg;
	int shift;
	u32 pd_pull;

	if (!chip)
		return -EINVAL;

	off = pin - chip->chip.base;
	shift = off * 2;
	reg = chip->base + 0x14;

	pd_pull = __raw_readl(reg);
	pd_pull = pd_pull >> shift;
	pd_pull &= 0x3;

	return (__force s5p_gpio_pd_pull_t)pd_pull;
}
EXPORT_SYMBOL(s5p_gpio_get_pd_pull);

int s5p_gpio_set_pd_pull(unsigned int pin, s5p_gpio_pd_pull_t pd_pull)
{
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip(pin);
	unsigned int off;
	void __iomem *reg;
	int shift;
	u32 tmp;

	if (!chip)
		return -EINVAL;

	off = pin - chip->chip.base;
	shift = off * 2;
	reg = chip->base + 0x14;

	tmp = __raw_readl(reg);
	tmp &= ~(0x3 << shift);
	tmp |= pd_pull << shift;

	__raw_writel(tmp, reg);

	return 0;
}
EXPORT_SYMBOL(s5p_gpio_set_pd_pull);
