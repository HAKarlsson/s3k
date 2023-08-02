#pragma once

/** Enable serial port. */
void uart_init(void);

/** Put char on serial port. */
int uart_putc(char c);

/** Get char from serial port. */
int uart_getc(void);
