#include "drivers/uart.h"

#include <stdint.h>

extern volatile uint64_t _uart[];

static inline void uart_init(void)
{
	_uart[2] = 1;
	_uart[3] = 1;
}

static inline int uart_putc(char c)
{
	while (_uart[0] < 0)
		;
	_uart[0] = c;
	return c;
}

static inline int uart_getc(void)
{
	int c;
	while ((c = _uart[1]) < 0)
		;
	return c;
}
