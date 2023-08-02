#include "uart.h"

extern volatile char _uart[];

void uart_init(void)
{
	/** TODO: Proper init code for uart */
}

int uart_putc(char c)
{
	_uart[0] = c;
	return c;
}

int uart_getc(void)
{
	while (!(_uart[5] & 0x1))
		;
	return _uart[0];
}
