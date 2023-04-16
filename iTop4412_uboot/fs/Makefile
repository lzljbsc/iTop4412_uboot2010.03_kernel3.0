subdirs-$(CONFIG_CMD_EXT2) += ext2
subdirs-$(CONFIG_CMD_FAT) += fat

SUBDIRS	:= $(subdirs-y)

$(obj).depend all:
	@for dir in $(SUBDIRS) ; do \
		$(MAKE) -C $$dir $@ ; done
