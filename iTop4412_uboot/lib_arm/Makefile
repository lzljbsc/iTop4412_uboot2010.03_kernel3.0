include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(ARCH).a
LIBGCC	= $(obj)libgcc.a

GLSOBJS	+= _ashldi3.o
GLSOBJS	+= _ashrdi3.o
GLSOBJS	+= _divsi3.o
GLSOBJS	+= _lshrdi3.o
GLSOBJS	+= _modsi3.o
GLSOBJS	+= _udivsi3.o
GLSOBJS	+= _umodsi3.o

GLCOBJS	+= div0.o

COBJS-y	+= board.o
COBJS-y	+= bootm.o
COBJS-y	+= cache.o
ifndef CONFIG_SYS_NO_CP15_CACHE
COBJS-y	+= cache-cp15.o
endif
COBJS-y	+= interrupts.o
COBJS-y	+= reset.o

SRCS	:= $(GLSOBJS:.o=.S) $(GLCOBJS:.o=.c) \
	   $(SOBJS-y:.o=.S) $(COBJS-y:.o=.c)
OBJS	:= $(addprefix $(obj),$(SOBJS-y) $(COBJS-y))
LGOBJS	:= $(addprefix $(obj),$(GLSOBJS)) \
	   $(addprefix $(obj),$(GLCOBJS))

# Always build libarm.a
TARGETS	:= $(LIB)

# Build private libgcc only when asked for
ifdef USE_PRIVATE_LIBGCC
TARGETS	+= $(LIBGCC)
endif

# For EABI conformant tool chains, provide eabi_compat()
ifneq (,$(findstring -mabi=aapcs-linux,$(PLATFORM_CPPFLAGS)))
TARGETS	+= $(obj)eabi_compat.o
endif

all:	$(TARGETS)

$(LIB):	$(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

$(LIBGCC): $(obj).depend $(LGOBJS)
	$(AR) $(ARFLAGS) $@ $(LGOBJS)

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
