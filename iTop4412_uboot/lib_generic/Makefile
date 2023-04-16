include $(TOPDIR)/config.mk

LIB	= $(obj)libgeneric.a

COBJS-y += crc16.o
COBJS-y += crc32.o
COBJS-y += ctype.o
COBJS-y += display_options.o
COBJS-y += div64.o
COBJS-$(CONFIG_GZIP) += gunzip.o
COBJS-y += ldiv.o
COBJS-y += net_utils.o
COBJS-y += string.o
COBJS-y	+= strmhz.o
COBJS-y += time.o
COBJS-y += vsprintf.o
COBJS-$(CONFIG_ZLIB) += zlib.o

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

$(LIB):	$(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
