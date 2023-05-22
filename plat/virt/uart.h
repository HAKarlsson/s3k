#pragma once

#define UART ((volatile char *)0x10000000)

static inline void uart_enable(void)
{
}

static inline int uart_putchar(char c)
{
	UART[0] = c;
	return c;
}

static inline int uart_getchar(void)
{
	while (!(UART[5] & 0x1))
		;
	return UART[0];
}
