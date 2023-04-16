include $(TOPDIR)/config.mk

LIB	= $(obj)libcommon.a

AOBJS	=

# core
COBJS-y += main.o
COBJS-$(CONFIG_S3C_USBD) += cmd_usbd.o
COBJS-$(CONFIG_FASTBOOT) += cmd_fastboot.o
COBJS-y += console.o
COBJS-y += command.o
COBJS-y += dlmalloc.o
COBJS-y += exports.o
COBJS-y += image.o
COBJS-y += memsize.o
COBJS-y += s_record.o
COBJS-y += stdio.o
COBJS-y += xyzModem.o

# core command
COBJS-y += cmd_boot.o
COBJS-y += cmd_bootm.o
COBJS-y += cmd_help.o
COBJS-y += cmd_nvedit.o
COBJS-y += cmd_version.o

# environment
COBJS-y += env_common.o
COBJS-$(CONFIG_ENV_IS_IN_AUTO) += env_auto.o

# command
COBJS-$(CONFIG_SOURCE) += cmd_source.o
COBJS-$(CONFIG_CMD_SOURCE) += cmd_source.o
COBJS-$(CONFIG_CMD_BDI) += cmd_bdinfo.o
COBJS-$(CONFIG_CMD_CONSOLE) += cmd_console.o
COBJS-$(CONFIG_CMD_ECHO) += cmd_echo.o
COBJS-$(CONFIG_CMD_EXT2) += cmd_ext2.o
COBJS-$(CONFIG_CMD_EXT4) += decompress_ext4.o
COBJS-$(CONFIG_CMD_FAT) += cmd_fat.o
COBJS-$(CONFIG_CMD_ITEST) += cmd_itest.o
COBJS-y += cmd_load.o
COBJS-$(CONFIG_CMD_MEMORY) += cmd_mem.o
COBJS-$(CONFIG_CMD_MISC) += cmd_misc.o
COBJS-$(CONFIG_CMD_MMC) += cmd_mmc.o cmd_mmc_fdisk.o
ifndef CONFIG_SECURE
COBJS-y	+= cmd_movi.o
endif
COBJS-$(CONFIG_CMD_XIMG) += cmd_ximg.o

# others
COBJS-y += flash.o
COBJS-y += ace_sha1.o
COBJS-y += cmd_mdmupgrade.o

COBJS	:= $(sort $(COBJS-y))
SRCS	:= $(AOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(AOBJS) $(COBJS))

CPPFLAGS += -I..

all:	$(LIB) $(AOBJS)

$(LIB): $(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

$(obj)env_embedded.o: $(src)env_embedded.c $(obj)../tools/envcrc
	$(CC) $(AFLAGS) -Wa,--no-warn \
		-DENV_CRC=$(shell $(obj)../tools/envcrc) \
		-c -o $@ $(src)env_embedded.c

$(obj)../tools/envcrc:
	$(MAKE) -C ../tools

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
