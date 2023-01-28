#
# (C) Copyright 2000-2003
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

# 包含了顶层的 config.mk ，就包含了之前到处的各种变量
# 留意 config.mk 中最后一个段落，make 的隐含规则
# 隐含规则为每个源码文件创建了一个目标
# 所以这里的 start.o ，在顶层 Makefile 中，是通过 make -C cpu/arm_cortexa9 start.o 
# 执行的，就是 $(OBJS) 目标
include $(TOPDIR)/config.mk

# 这里的 LIB 定义，需要与顶层 Makefile 中的 LIBS 定义一致
LIB	= $(obj)lib$(CPU).a

# START 是这个 Makefile 中比较特殊的一个，其他的没有
# COBJS 是所有的 C 源码文件目标
START	:= start.o
COBJS	:= cpu.o

# SRCS 是所有的源码文件，在下面包含的 rules.mk 中使用，用于生成 .depend 
# OBJS 是所有C源码目标文件
SRCS	:= $(START:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))
START	:= $(addprefix $(obj),$(START))

all:	$(obj).depend $(START) $(LIB)

# 生成 lib 的目标,每个目录下的 Makefile 都有这个，
# 编译的方式就是将所有目录的源码编译成 lib，再统一链接
$(LIB):	$(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

#########################################################################

# defines $(obj).depend target
# 通过上面的 SRCS 自动生成依赖关系目标， _depend 
# 通过执行 make _depend 可生成该目录下源文件的依赖  .depend
include $(SRCTREE)/rules.mk

# 包含源码的依赖关系，源文件依赖的头文件，这样如果有头文件改动
# 则包含该头文件的源码就会编译
# 这个依赖关闭是通过上面的 rules.mk 自动生成的,  _depend 目标
sinclude $(obj).depend

#########################################################################
