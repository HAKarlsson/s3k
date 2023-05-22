#include "s3k.h"

// Syscall
uint64_t s3k_getpid(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GETINFO;
	register uint64_t a0 __asm__("a0") = 0;
	__asm__ volatile("ecall"
			 : "+r"(t0), "+r"(a0));
	return a0;
}

uint64_t s3k_gethartid(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GETINFO;
	register uint64_t a0 __asm__("a0") = 1;
	__asm__ volatile("ecall"
			 : "+r"(t0), "+r"(a0));
	return a0;
}

uint64_t s3k_gettime(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GETINFO;
	register uint64_t a0 __asm__("a1") = 2;
	__asm__ volatile("ecall"
			 : "+r"(t0), "+r"(a0));
	return a0;
}

uint64_t s3k_gettimeout(void)
{
	register uint64_t t0 __asm__("a0") = S3K_SYSCALL_GETINFO;
	register uint64_t a0 __asm__("t0") = 3;
	__asm__ volatile("ecall"
			 : "+r"(t0), "+r"(a0));
	return a0;
}

uint64_t s3k_getreg(enum s3k_reg reg)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GETREG;
	register uint64_t a0 __asm__("a0") = 2;
	__asm__ volatile("ecall"
			 : "+r"(t0), "+r"(a0));
	return a0;
}

void s3k_setreg(enum s3k_reg reg, uint64_t val)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_SETREG;
	register uint64_t a0 __asm__("a0") = reg;
	register uint64_t a1 __asm__("a1") = val;
	__asm__ volatile("ecall" ::"r"(t0), "r"(a0), "r"(a1));
}

void s3k_yield(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_YIELD;
	__asm__("ecall"
		:
		: "r"(t0));
}

// void s3k_suspend(void)
//{
//	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_PROC;
//	register uint64_t a1 __asm__("a1") = 7;
//	__asm__("ecall"
//		:
//		: "r"(t0), "r"(a1));
// }

enum s3k_excpt s3k_getcap(uint64_t i, union s3k_cap *cap)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GETCAP;
	register uint64_t a0 __asm__("a0") = i;
	__asm__ volatile("ecall"
			 : "+r"(a0), "+r"(t0));
	if (t0 == S3K_EXCPT_NONE)
		cap->raw = a0;
	return t0;
}

enum s3k_excpt s3k_movcap(uint64_t src, uint64_t dst)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MOVCAP;
	register uint64_t a0 __asm__("a0") = src;
	register uint64_t a1 __asm__("a1") = dst;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0), "r"(a1));
	return t0;
}

enum s3k_excpt s3k_delcap(uint64_t i)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_DELCAP;
	register uint64_t a0 __asm__("a0") = i;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0));
	return t0;
}

enum s3k_excpt s3k_revcap(uint64_t i)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_REVCAP;
	register uint64_t a0 __asm__("a0") = i;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0));
	return t0;
}

enum s3k_excpt s3k_drvcap(uint64_t i, uint64_t j, union s3k_cap cap)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_DRVCAP;
	register uint64_t a0 __asm__("a0") = i;
	register uint64_t a1 __asm__("a1") = j;
	register uint64_t a2 __asm__("a2") = cap.raw;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0), "r"(a1), "r"(a2));
	return t0;
}

enum s3k_excpt s3k_msuspend(uint64_t i, uint64_t pid)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MSUSPEND;
	register uint64_t a0 __asm__("a0") = i;
	register uint64_t a1 __asm__("a1") = pid;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0), "r"(a1));
	return t0;
}

enum s3k_excpt s3k_mresume(uint64_t i, uint64_t pid)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MRESUME;
	register uint64_t a0 __asm__("a0") = i;
	register uint64_t a1 __asm__("a1") = pid;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0), "r"(a1));
	return t0;
}

enum s3k_excpt s3k_mgetreg(uint64_t i, uint64_t pid, enum s3k_reg reg, uint64_t *val)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MGETREG;
	register uint64_t a0 __asm__("a0") = i;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = reg;
	__asm__ volatile("ecall"
			 : "+r"(t0), "+r"(a0)
			 : "r"(a1), "r"(a2));
	if (t0 == S3K_EXCPT_NONE)
		*val = a1;
	return t0;
}

enum s3k_excpt s3k_msetreg(uint64_t i, uint64_t pid, enum s3k_reg reg, uint64_t val)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MSETREG;
	register uint64_t a0 __asm__("a0") = i;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = reg;
	register uint64_t a3 __asm__("a3") = val;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0), "r"(a1), "r"(a2), "r"(a3));
	return t0;
}

enum s3k_excpt s3k_mgetcap(uint64_t i, uint64_t pid, uint64_t j, union s3k_cap *cap)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MGETCAP;
	register uint64_t a0 __asm__("a0") = i;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = j;
	__asm__ volatile("ecall"
			 : "+r"(t0), "+r"(a0)
			 : "r"(a1), "r"(a2));
	if (t0 == S3K_EXCPT_NONE)
		cap->raw = a1;
	return t0;
}

enum s3k_excpt s3k_mtakecap(uint64_t i, uint64_t pid, uint64_t src, uint64_t dst)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MTAKECAP;
	register uint64_t a0 __asm__("a0") = i;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = src;
	register uint64_t a3 __asm__("a3") = dst;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0), "r"(a1), "r"(a2), "r"(a3));
	return t0;
}

enum s3k_excpt s3k_mgivecap(uint64_t i, uint64_t pid, uint64_t src, uint64_t dst)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MGIVECAP;
	register uint64_t a0 __asm__("a0") = i;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = src;
	register uint64_t a3 __asm__("a3") = dst;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0), "r"(a1), "r"(a2), "r"(a3));
	return t0;
}

enum s3k_excpt s3k_recv(uint64_t i, uint64_t buf[4], uint64_t cap_dest, uint64_t *tag)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_RECV;
	register uint64_t a0 __asm__("a0") = i;
	register uint64_t a1 __asm__("a1");
	register uint64_t a2 __asm__("a2");
	register uint64_t a3 __asm__("a3");
	register uint64_t a4 __asm__("a4") = cap_dest;
	__asm__ volatile("ecall"
			 : "+r"(t0), "+r"(a0), "=r"(a1), "=r"(a2), "=r"(a3), "+r"(a4));
	if (t0 != S3K_EXCPT_NONE)
		return t0;
	buf[0] = a1;
	buf[1] = a2;
	buf[2] = a3;
	buf[3] = a4;
	*tag = a0;
	return S3K_EXCPT_NONE;
}

enum s3k_excpt s3k_send(uint64_t i, uint64_t buf[4], uint64_t cap_src, bool yield)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_SEND;
	register uint64_t a0 __asm__("a0") = i;
	register uint64_t a1 __asm__("a1") = buf[0];
	register uint64_t a2 __asm__("a2") = buf[1];
	register uint64_t a3 __asm__("a3") = buf[2];
	register uint64_t a4 __asm__("a4") = buf[3];
	register uint64_t a5 __asm__("a5") = cap_src << 8 | yield << 16;
	register uint64_t a6 __asm__("a6") = yield;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
			   "r"(a6));
	return t0;
}

enum s3k_excpt s3k_sendrecv(uint64_t i, uint64_t j, uint64_t buf[4],
			    uint64_t cap_src, uint64_t cap_dest, uint64_t *tag)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_SENDRECV;
	register uint64_t a0 __asm__("a0") = i << 16 | j;
	register uint64_t a1 __asm__("a1") = buf[0];
	register uint64_t a2 __asm__("a2") = buf[1];
	register uint64_t a3 __asm__("a3") = buf[2];
	register uint64_t a4 __asm__("a4") = buf[3];
	register uint64_t a5 __asm__("a5") = cap_dest | (cap_src << 8);
	__asm__ volatile("ecall"
			 : "+r"(t0), "+r"(a0), "+r"(a1), "+r"(a2), "+r"(a3), "+r"(a4)
			 : "r"(a5));
	if (t0 != S3K_EXCPT_NONE)
		return t0;
	buf[0] = a1;
	buf[1] = a2;
	buf[2] = a3;
	buf[3] = a4;
	*tag = a0;
	return t0;
}
