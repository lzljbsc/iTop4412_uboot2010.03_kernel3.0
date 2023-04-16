include $(TOPDIR)/config.mk

#CFLAGS += -DET_DEBUG -DDEBUG

LIB	= $(obj)libdisk.a

COBJS-y += part.o
COBJS-$(CONFIG_DOS_PARTITION)   += part_dos.o

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
