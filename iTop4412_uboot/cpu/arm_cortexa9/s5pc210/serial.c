#include <common.h>

#include <s5pc210.h>

#define UART_NR	S5PC21X_UART2

void serial_setbrg(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	int i;
	for (i = 0; i < 100; i++);
}

/*
 * Initialise the serial port with the given baudrate. The settings
 * are always 8 data bits, no parity, 1 stop bit, no start bits.
 *
 */
int serial_init(void)
{
    /* 初始化串口，但这里什么也没做
     * 是在前面的汇编启动过程中已初始化完成，这里直接用就可以了 */
	serial_setbrg();

	return (0);
}

/*
 * Read a single byte from the serial port. Returns 1 on success, 0
 * otherwise. When the function is succesfull, the character read is
 * written into its argument c.
 */
int serial_getc(void)
{
	S5PC21X_UART *const uart = S5PC21X_GetBase_UART(UART_NR);

	/* wait for character to arrive */
	while (!(uart->UTRSTAT & 0x1));

	return uart->URXH & 0xff;
}


/*
 * Output a single byte to the serial port.
 */
void serial_putc(const char c)
{
	S5PC21X_UART *const uart = S5PC21X_GetBase_UART(UART_NR);

	/* wait for room in the tx FIFO */
	while (!(uart->UTRSTAT & 0x2));

	uart->UTXH = c;

	/* If \n, also do \r */
	if (c == '\n')
		serial_putc('\r');
}

/*
 * Test whether a character is in the RX buffer
 */
int serial_tstc(void)
{
	S5PC21X_UART *const uart = S5PC21X_GetBase_UART(UART_NR);

	return uart->UTRSTAT & 0x1;
}

void serial_puts(const char *s)
{
	while (*s) {
		serial_putc(*s++);
	}
}

