#include "altio.h"
#include "s3k.h"

int main(void)
{
	s3k_drvcap(2, 16, s3k_mkpmp(0x40001ff, S3K_RW));
	s3k_pmpset(16, 1);
	alt_puts("hello, world");
}
