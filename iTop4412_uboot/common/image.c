
#include <common.h>
#include <watchdog.h>

#include <image.h>

#ifdef CONFIG_CMD_BDI
extern int do_bdinfo(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
#endif

DECLARE_GLOBAL_DATA_PTR;

static const image_header_t* image_get_ramdisk (ulong rd_addr, uint8_t arch,
						int verify);

static table_entry_t uimage_arch[] = {
	{	IH_ARCH_INVALID,	NULL,		"Invalid ARCH",	},
	{	IH_ARCH_ALPHA,		"alpha",	"Alpha",	},
	{	IH_ARCH_ARM,		"arm",		"ARM",		},
	{	IH_ARCH_I386,		"x86",		"Intel x86",	},
	{	IH_ARCH_IA64,		"ia64",		"IA64",		},
	{	IH_ARCH_M68K,		"m68k",		"M68K",		},
	{	IH_ARCH_MICROBLAZE,	"microblaze",	"MicroBlaze",	},
	{	IH_ARCH_MIPS,		"mips",		"MIPS",		},
	{	IH_ARCH_MIPS64,		"mips64",	"MIPS 64 Bit",	},
	{	IH_ARCH_NIOS,		"nios",		"NIOS",		},
	{	IH_ARCH_NIOS2,		"nios2",	"NIOS II",	},
	{	IH_ARCH_PPC,		"powerpc",	"PowerPC",	},
	{	IH_ARCH_PPC,		"ppc",		"PowerPC",	},
	{	IH_ARCH_S390,		"s390",		"IBM S390",	},
	{	IH_ARCH_SH,		"sh",		"SuperH",	},
	{	IH_ARCH_SPARC,		"sparc",	"SPARC",	},
	{	IH_ARCH_SPARC64,	"sparc64",	"SPARC 64 Bit",	},
	{	IH_ARCH_BLACKFIN,	"blackfin",	"Blackfin",	},
	{	IH_ARCH_AVR32,		"avr32",	"AVR32",	},
	{	-1,			"",		"",		},
};

static table_entry_t uimage_os[] = {
	{	IH_OS_INVALID,	NULL,		"Invalid OS",		},
	{	IH_OS_LINUX,	"linux",	"Linux",		},
	{	IH_OS_NETBSD,	"netbsd",	"NetBSD",		},
	{	IH_OS_RTEMS,	"rtems",	"RTEMS",		},
	{	IH_OS_U_BOOT,	"u-boot",	"U-Boot",		},
	{	-1,		"",		"",			},
};

static table_entry_t uimage_type[] = {
	{	IH_TYPE_INVALID,    NULL,	  "Invalid Image",	},
	{	IH_TYPE_FILESYSTEM, "filesystem", "Filesystem Image",	},
	{	IH_TYPE_FIRMWARE,   "firmware",	  "Firmware",		},
	{	IH_TYPE_KERNEL,	    "kernel",	  "Kernel Image",	},
	{	IH_TYPE_MULTI,	    "multi",	  "Multi-File Image",	},
	{	IH_TYPE_RAMDISK,    "ramdisk",	  "RAMDisk Image",	},
	{	IH_TYPE_SCRIPT,     "script",	  "Script",		},
	{	IH_TYPE_STANDALONE, "standalone", "Standalone Program", },
	{	IH_TYPE_FLATDT,     "flat_dt",    "Flat Device Tree",	},
	{	IH_TYPE_KWBIMAGE,   "kwbimage",   "Kirkwood Boot Image",},
	{	IH_TYPE_IMXIMAGE,   "imximage",   "Freescale i.MX Boot Image",},
	{	-1,		    "",		  "",			},
};

static table_entry_t uimage_comp[] = {
	{	IH_COMP_NONE,	"none",		"uncompressed",		},
	{	IH_COMP_BZIP2,	"bzip2",	"bzip2 compressed",	},
	{	IH_COMP_GZIP,	"gzip",		"gzip compressed",	},
	{	IH_COMP_LZMA,	"lzma",		"lzma compressed",	},
	{	IH_COMP_LZO,	"lzo",		"lzo compressed",	},
	{	-1,		"",		"",			},
};

uint32_t crc32 (uint32_t, const unsigned char *, uint);
uint32_t crc32_wd (uint32_t, const unsigned char *, uint, uint);

/*****************************************************************************/
/* Legacy format routines */
/*****************************************************************************/
int image_check_hcrc (const image_header_t *hdr)
{
	ulong hcrc;
	ulong len = image_get_header_size ();
	image_header_t header;

	/* Copy header so we can blank CRC field for re-calculation */
	memmove (&header, (char *)hdr, image_get_header_size ());
	image_set_hcrc (&header, 0);

	hcrc = crc32 (0, (unsigned char *)&header, len);

	return (hcrc == image_get_hcrc (hdr));
}

int image_check_dcrc (const image_header_t *hdr)
{
	ulong data = image_get_data (hdr);
	ulong len = image_get_data_size (hdr);
	ulong dcrc = crc32_wd (0, (unsigned char *)data, len, CHUNKSZ_CRC32);

	return (dcrc == image_get_dcrc (hdr));
}

/**
 * image_multi_count - get component (sub-image) count
 * @hdr: pointer to the header of the multi component image
 *
 * image_multi_count() returns number of components in a multi
 * component image.
 *
 * Note: no checking of the image type is done, caller must pass
 * a valid multi component image.
 *
 * returns:
 *     number of components
 */
ulong image_multi_count (const image_header_t *hdr)
{
	ulong i, count = 0;
	uint32_t *size;

	/* get start of the image payload, which in case of multi
	 * component images that points to a table of component sizes */
	size = (uint32_t *)image_get_data (hdr);

	/* count non empty slots */
	for (i = 0; size[i]; ++i)
		count++;

	return count;
}

/**
 * image_multi_getimg - get component data address and size
 * @hdr: pointer to the header of the multi component image
 * @idx: index of the requested component
 * @data: pointer to a ulong variable, will hold component data address
 * @len: pointer to a ulong variable, will hold component size
 *
 * image_multi_getimg() returns size and data address for the requested
 * component in a multi component image.
 *
 * Note: no checking of the image type is done, caller must pass
 * a valid multi component image.
 *
 * returns:
 *     data address and size of the component, if idx is valid
 *     0 in data and len, if idx is out of range
 */
void image_multi_getimg (const image_header_t *hdr, ulong idx,
			ulong *data, ulong *len)
{
	int i;
	uint32_t *size;
	ulong offset, count, img_data;

	/* get number of component */
	count = image_multi_count (hdr);

	/* get start of the image payload, which in case of multi
	 * component images that points to a table of component sizes */
	size = (uint32_t *)image_get_data (hdr);

	/* get address of the proper component data start, which means
	 * skipping sizes table (add 1 for last, null entry) */
	img_data = image_get_data (hdr) + (count + 1) * sizeof (uint32_t);

	if (idx < count) {
		*len = uimage_to_cpu (size[idx]);
		offset = 0;

		/* go over all indices preceding requested component idx */
		for (i = 0; i < idx; i++) {
			/* add up i-th component size, rounding up to 4 bytes */
			offset += (uimage_to_cpu (size[i]) + 3) & ~3 ;
		}

		/* calculate idx-th component data address */
		*data = img_data + offset;
	} else {
		*len = 0;
		*data = 0;
	}
}

static void image_print_type (const image_header_t *hdr)
{
	const char *os, *arch, *type, *comp;

	os = genimg_get_os_name (image_get_os (hdr));
	arch = genimg_get_arch_name (image_get_arch (hdr));
	type = genimg_get_type_name (image_get_type (hdr));
	comp = genimg_get_comp_name (image_get_comp (hdr));

	printf ("%s %s %s (%s)\n", arch, os, type, comp);
}

/**
 * image_print_contents - prints out the contents of the legacy format image
 * @ptr: pointer to the legacy format image header
 * @p: pointer to prefix string
 *
 * image_print_contents() formats a multi line legacy image contents description.
 * The routine prints out all header fields followed by the size/offset data
 * for MULTI/SCRIPT images.
 *
 * returns:
 *     no returned results
 */
void image_print_contents (const void *ptr)
{
	const image_header_t *hdr = (const image_header_t *)ptr;
	const char *p;

	p = "   ";

	printf ("%sImage Name:   %.*s\n", p, IH_NMLEN, image_get_name (hdr));
	printf ("%sImage Type:   ", p);
	image_print_type (hdr);
	printf ("%sData Size:    ", p);
	genimg_print_size (image_get_data_size (hdr));
	printf ("%sLoad Address: %08x\n", p, image_get_load (hdr));
	printf ("%sEntry Point:  %08x\n", p, image_get_ep (hdr));

	if (image_check_type (hdr, IH_TYPE_MULTI) ||
			image_check_type (hdr, IH_TYPE_SCRIPT)) {
		int i;
		ulong data, len;
		ulong count = image_multi_count (hdr);

		printf ("%sContents:\n", p);
		for (i = 0; i < count; i++) {
			image_multi_getimg (hdr, i, &data, &len);

			printf ("%s   Image %d: ", p, i);
			genimg_print_size (len);

			if (image_check_type (hdr, IH_TYPE_SCRIPT) && i > 0) {
				/*
				 * the user may need to know offsets
				 * if planning to do something with
				 * multiple files
				 */
				printf ("%s    Offset = 0x%08lx\n", p, data);
			}
		}
	}
}


/**
 * image_get_ramdisk - get and verify ramdisk image
 * @rd_addr: ramdisk image start address
 * @arch: expected ramdisk architecture
 * @verify: checksum verification flag
 *
 * image_get_ramdisk() returns a pointer to the verified ramdisk image
 * header. Routine receives image start address and expected architecture
 * flag. Verification done covers data and header integrity and os/type/arch
 * fields checking.
 *
 * If dataflash support is enabled routine checks for dataflash addresses
 * and handles required dataflash reads.
 *
 * returns:
 *     pointer to a ramdisk image header, if image was found and valid
 *     otherwise, return NULL
 */
static const image_header_t *image_get_ramdisk (ulong rd_addr, uint8_t arch,
						int verify)
{
	const image_header_t *rd_hdr = (const image_header_t *)rd_addr;

	if (!image_check_magic (rd_hdr)) {
		puts ("Bad Magic Number\n");
		show_boot_progress (-10);
		return NULL;
	}

	if (!image_check_hcrc (rd_hdr)) {
		puts ("Bad Header Checksum\n");
		show_boot_progress (-11);
		return NULL;
	}

	show_boot_progress (10);
	image_print_contents (rd_hdr);

	if (verify) {
		puts("   Verifying Checksum ... ");
		if (!image_check_dcrc (rd_hdr)) {
			puts ("Bad Data CRC\n");
			show_boot_progress (-12);
			return NULL;
		}
		puts("OK\n");
	}

	show_boot_progress (11);

	if (!image_check_os (rd_hdr, IH_OS_LINUX) ||
	    !image_check_arch (rd_hdr, arch) ||
	    !image_check_type (rd_hdr, IH_TYPE_RAMDISK)) {
		printf ("No Linux %s Ramdisk Image\n",
				genimg_get_arch_name(arch));
		show_boot_progress (-13);
		return NULL;
	}

	return rd_hdr;
}

/*****************************************************************************/
/* Shared dual-format routines */
/*****************************************************************************/
int getenv_yesno (char *var)
{
	char *s = getenv (var);
	return (s && (*s == 'n')) ? 0 : 1;
}

ulong getenv_bootm_low(void)
{
	char *s = getenv ("bootm_low");
	if (s) {
		ulong tmp = simple_strtoul (s, NULL, 16);
		return tmp;
	}

	return gd->bd->bi_dram[0].start;
}

phys_size_t getenv_bootm_size(void)
{
	char *s = getenv ("bootm_size");
	if (s) {
		phys_size_t tmp;
		tmp = (phys_size_t)simple_strtoull (s, NULL, 16);
		return tmp;
	}

	return gd->bd->bi_dram[0].size;
}

void memmove_wd (void *to, void *from, size_t len, ulong chunksz)
{
	memmove (to, from, len);
}

void genimg_print_size (uint32_t size)
{
	printf ("%d Bytes = ", size);
	print_size (size, "\n");
}

/**
 * get_table_entry_name - translate entry id to long name
 * @table: pointer to a translation table for entries of a specific type
 * @msg: message to be returned when translation fails
 * @id: entry id to be translated
 *
 * get_table_entry_name() will go over translation table trying to find
 * entry that matches given id. If matching entry is found, its long
 * name is returned to the caller.
 *
 * returns:
 *     long entry name if translation succeeds
 *     msg otherwise
 */
char *get_table_entry_name (table_entry_t *table, char *msg, int id)
{
	for (; table->id >= 0; ++table) {
		if (table->id == id)
			return table->lname;
	}
	return (msg);
}

const char *genimg_get_os_name (uint8_t os)
{
	return (get_table_entry_name (uimage_os, "Unknown OS", os));
}

const char *genimg_get_arch_name (uint8_t arch)
{
	return (get_table_entry_name (uimage_arch, "Unknown Architecture", arch));
}

const char *genimg_get_type_name (uint8_t type)
{
	return (get_table_entry_name (uimage_type, "Unknown Image", type));
}

const char *genimg_get_comp_name (uint8_t comp)
{
	return (get_table_entry_name (uimage_comp, "Unknown Compression", comp));
}

/**
 * get_table_entry_id - translate short entry name to id
 * @table: pointer to a translation table for entries of a specific type
 * @table_name: to be used in case of error
 * @name: entry short name to be translated
 *
 * get_table_entry_id() will go over translation table trying to find
 * entry that matches given short name. If matching entry is found,
 * its id returned to the caller.
 *
 * returns:
 *     entry id if translation succeeds
 *     -1 otherwise
 */
int get_table_entry_id (table_entry_t *table,
		const char *table_name, const char *name)
{
	table_entry_t *t;
	for (t = table; t->id >= 0; ++t) {
		if (t->sname && strcmp(t->sname, name) == 0)
			return (t->id);
	}
	debug ("Invalid %s Type: %s\n", table_name, name);
	return (-1);
}

int genimg_get_os_id (const char *name)
{
	return (get_table_entry_id (uimage_os, "OS", name));
}

int genimg_get_arch_id (const char *name)
{
	return (get_table_entry_id (uimage_arch, "CPU", name));
}

int genimg_get_type_id (const char *name)
{
	return (get_table_entry_id (uimage_type, "Image", name));
}

int genimg_get_comp_id (const char *name)
{
	return (get_table_entry_id (uimage_comp, "Compression", name));
}

/**
 * genimg_get_format - get image format type
 * @img_addr: image start address
 *
 * genimg_get_format() checks whether provided address points to a valid
 * legacy or FIT image.
 *
 * New uImage format and FDT blob are based on a libfdt. FDT blob
 * may be passed directly or embedded in a FIT image. In both situations
 * genimg_get_format() must be able to dectect libfdt header.
 *
 * returns:
 *     image format type or IMAGE_FORMAT_INVALID if no image is present
 */
int genimg_get_format (void *img_addr)
{
	ulong format = IMAGE_FORMAT_INVALID;
	const image_header_t *hdr;

	hdr = (const image_header_t *)img_addr;
	if (image_check_magic(hdr))
		format = IMAGE_FORMAT_LEGACY;

	return format;
}

/**
 * genimg_get_image - get image from special storage (if necessary)
 * @img_addr: image start address
 *
 * genimg_get_image() checks if provided image start adddress is located
 * in a dataflash storage. If so, image is moved to a system RAM memory.
 *
 * returns:
 *     image start address after possible relocation from special storage
 */
ulong genimg_get_image (ulong img_addr)
{
	ulong ram_addr = img_addr;

	return ram_addr;
}

/**
 * fit_has_config - check if there is a valid FIT configuration
 * @images: pointer to the bootm command headers structure
 *
 * fit_has_config() checks if there is a FIT configuration in use
 * (if FTI support is present).
 *
 * returns:
 *     0, no FIT support or no configuration found
 *     1, configuration found
 */
int genimg_has_config (bootm_headers_t *images)
{
	return 0;
}

/**
 * boot_get_ramdisk - main ramdisk handling routine
 * @argc: command argument count
 * @argv: command argument list
 * @images: pointer to the bootm images structure
 * @arch: expected ramdisk architecture
 * @rd_start: pointer to a ulong variable, will hold ramdisk start address
 * @rd_end: pointer to a ulong variable, will hold ramdisk end
 *
 * boot_get_ramdisk() is responsible for finding a valid ramdisk image.
 * Curently supported are the following ramdisk sources:
 *      - multicomponent kernel/ramdisk image,
 *      - commandline provided address of decicated ramdisk image.
 *
 * returns:
 *     0, if ramdisk image was found and valid, or skiped
 *     rd_start and rd_end are set to ramdisk start/end addresses if
 *     ramdisk image is found and valid
 *
 *     1, if ramdisk image is found but corrupted, or invalid
 *     rd_start and rd_end are set to 0 if no ramdisk exists
 */
int boot_get_ramdisk (int argc, char *argv[], bootm_headers_t *images,
		uint8_t arch, ulong *rd_start, ulong *rd_end)
{
	ulong rd_addr, rd_load;
	ulong rd_data, rd_len;
	const image_header_t *rd_hdr;

	*rd_start = 0;
	*rd_end = 0;

    // TODO: 确认下传进来的具体参数
	/*
	 * Look for a '-' which indicates to ignore the
	 * ramdisk argument
	 */
	if ((argc >= 3) && (strcmp(argv[2], "-") ==  0)) {
		debug ("## Skipping init Ramdisk\n");
		rd_len = rd_data = 0;
	} else if (argc >= 3 || genimg_has_config (images)) {
			{
				rd_addr = simple_strtoul(argv[2], NULL, 16);
				debug ("*  ramdisk: cmdline image address = 0x%08lx\n",
						rd_addr);
			}

		/* copy from dataflash if needed */
		rd_addr = genimg_get_image (rd_addr);

		/*
		 * Check if there is an initrd image at the
		 * address provided in the second bootm argument
		 * check image type, for FIT images get FIT node.
		 */
		switch (genimg_get_format ((void *)rd_addr)) {
		case IMAGE_FORMAT_LEGACY:
			printf ("## Loading init Ramdisk from Legacy "
					"Image at %08lx ...\n", rd_addr);

			show_boot_progress (9);
			rd_hdr = image_get_ramdisk (rd_addr, arch,
							images->verify);

			if (rd_hdr == NULL)
				return 1;

			rd_data = image_get_data (rd_hdr);
			rd_len = image_get_data_size (rd_hdr);
			rd_load = image_get_load (rd_hdr);
			break;
		default:
			puts ("Wrong Ramdisk Image Format\n");
			rd_data = rd_len = rd_load = 0;
			return 1;
		}

	} else if (images->legacy_hdr_valid &&
			image_check_type (&images->legacy_hdr_os_copy, IH_TYPE_MULTI)) {
		/*
		 * Now check if we have a legacy mult-component image,
		 * get second entry data start address and len.
		 */
		show_boot_progress (13);
		printf ("## Loading init Ramdisk from multi component "
				"Legacy Image at %08lx ...\n",
				(ulong)images->legacy_hdr_os);

		image_multi_getimg (images->legacy_hdr_os, 1, &rd_data, &rd_len);
	} else {
		/*
		 * no initrd image
		 */
		show_boot_progress (14);
		rd_len = rd_data = 0;
	}

	if (!rd_data) {
		debug ("## No init Ramdisk\n");
	} else {
		*rd_start = rd_data;
		*rd_end = rd_data + rd_len;
	}
	debug ("   ramdisk start = 0x%08lx, ramdisk end = 0x%08lx\n",
			*rd_start, *rd_end);

	return 0;
}

