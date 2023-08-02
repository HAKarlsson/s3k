#include "altio.h"
#include "uart.h"

void info(void)
{
	uart_init();
	alt_puts("Kernel initialization complete");
}
