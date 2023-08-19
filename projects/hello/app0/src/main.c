#include "altio.h"
#include "s3k.h"

int main(void)
{
	uint64_t uart_addr = s3k_napot_encode(0x10000000, 0x8);
	s3k_drvcap(2, 16, s3k_mkpmp(uart_addr, S3K_RW));
	s3k_pmpset(16, 1);
	alt_puts("hello, world");
}
