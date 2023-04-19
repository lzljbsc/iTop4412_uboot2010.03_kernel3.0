
#include <config.h>
#include <common.h>
#include <linux/ctype.h>
#include <asm/io.h>

int display_options (void)
{
	extern char version_string[];
	printf ("\n\n%s\n\n", version_string);

	return 0;
}

/*
 * print sizes as "xxx kB", "xxx.y kB", "xxx MB", "xxx.y MB",
 * xxx GB, or xxx.y GB as needed; allow for optional trailing string
 * (like "\n")
 */
void print_size (phys_size_t size, const char *s)
{
	ulong m = 0, n;
	phys_size_t d = 1 << 30;		/* 1 GB */
	char  c = 'G';

	if (size < d) {			/* try MB */
		c = 'M';
		d = 1 << 20;
		if (size < d) {		/* print in kB */
			c = 'k';
			d = 1 << 10;
		}
	}

	n = size / d;

	/* If there's a remainder, deal with it */
	if(size % d) {
		m = (10 * (size - (n * d)) + (d / 2) ) / d;

		if (m >= 10) {
			m -= 10;
			n += 1;
		}
	}

	printf ("%2ld", n);
	if (m) {
		printf (".%ld", m);
	}
	printf (" %cB%s", c, s);
}

/*
 * Print data buffer in hex and ascii form to the terminal.
 *
 * data reads are buffered so that each memory address is only read once.
 * Useful when displaying the contents of volatile registers.
 *
 * parameters:
 *    addr: Starting address to display at start of line
 *    data: pointer to data buffer
 *    width: data value width.  May be 1, 2, or 4.
 *    count: number of values to display
 *    linelen: Number of values to print per line; specify 0 for default length
 */
#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)
int print_buffer (ulong addr, void* data, uint width, uint count, uint linelen)
{
	uint8_t linebuf[MAX_LINE_LENGTH_BYTES];
	uint32_t *uip = (void*)linebuf;
	uint16_t *usp = (void*)linebuf;
	uint8_t *ucp = (void*)linebuf;
	int i;

	if (linelen*width > MAX_LINE_LENGTH_BYTES)
		linelen = MAX_LINE_LENGTH_BYTES / width;
	if (linelen < 1)
		linelen = DEFAULT_LINE_LENGTH_BYTES / width;

	while (count) {
		printf("%08lx:", addr);

		/* check for overflow condition */
		if (count < linelen)
			linelen = count;

		/* Copy from memory into linebuf and print hex values */
		for (i = 0; i < linelen; i++) {
			if (width == 4) {
				uip[i] = *(volatile uint32_t *)data;
				printf(" %08x", uip[i]);
			} else if (width == 2) {
				usp[i] = *(volatile uint16_t *)data;
				printf(" %04x", usp[i]);
			} else {
				ucp[i] = *(volatile uint8_t *)data;
				printf(" %02x", ucp[i]);
			}
			data += width;
		}

		/* Print data in ASCII characters */
		puts("    ");
		for (i = 0; i < linelen * width; i++)
			putc(isprint(ucp[i]) && (ucp[i] < 0x80) ? ucp[i] : '.');
		putc ('\n');

		/* update references */
		addr += linelen * width;
		count -= linelen;

		if (ctrlc())
			return -1;
	}

	return 0;
}
