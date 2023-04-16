include $(TOPDIR)/config.mk

LIB	:= $(obj)libmmc.a

COBJS-$(CONFIG_GENERIC_MMC) += mmc.o
COBJS-$(CONFIG_S3C_HSMMC) += s3c_hsmmc.o
COBJS-$(CONFIG_S5P_MSHC) += s5p_mshc.o

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB): $(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
