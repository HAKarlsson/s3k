#pragma once

#define UART ((volatile int *)0x10010000ull)

static inline void uart_enable(void)
{
	UART[2] = 1;
	UART[3] = 1;
}

static inline int uart_putchar(char c)
{
	while (UART[0] < 0)
		;
	UART[0] = c;
	return c;
}

static inline int uart_getchar(void)
{
	int c;
	while ((c = UART[1]) < 0)
		;
	return c;
}
