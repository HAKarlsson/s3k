#include "altio.h"
#include "uart.h"

void info(void)
{
	uart_enable();
	alt_puts("Kernel initialization complete");
}
