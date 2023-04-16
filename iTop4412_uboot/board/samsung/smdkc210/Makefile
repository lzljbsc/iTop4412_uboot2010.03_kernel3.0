include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(BOARD).a

COBJS-y	:= smdkc210.o


SOBJS	:= lowlevel_init.o


SRCS    := $(SOBJS:.o=.S) $(COBJS-y:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS-y))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

$(LIB):	$(obj).depend $(SOBJS) $(OBJS)
	$(AR) $(ARFLAGS) $@ $(SOBJS) $(OBJS)

clean:
	rm -f $(SOBJS) $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak $(obj).depend

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
