#include <stdio.h>

extern volatile int __uart_base[]; // UART base address

#if UART_REG_STRIDE == 1
typedef char uart_reg_t;
#elif UART_REG_STRIDE == 4
typedef int uart_reg_t;
#else
#error "UART_REG_STRIDE must be defined (1 or 4)."
#endif

struct __attribute__((packed)) uart_regs {
	union {
		uart_reg_t rbr;
		uart_reg_t thr;
		uart_reg_t dll;
	};

	union {
		uart_reg_t ier;
		uart_reg_t dlm;
	};

	union {
		uart_reg_t iir;
		uart_reg_t fcr;
		uart_reg_t efr;
	};

	uart_reg_t lcr;

	union {
		uart_reg_t mcr;
		uart_reg_t xon1;
	};

	union {
		uart_reg_t lsr;
		uart_reg_t xon2;
	};

	union {
		uart_reg_t msr;
		uart_reg_t xoff1;
	};

	union {
		uart_reg_t spr;
		uart_reg_t xoff2;
	};
};

int __uart_putc(char c, FILE *f)
{
	(void)f;
	volatile struct uart_regs *regs = (struct uart_regs *)__uart_base;
	while (!(regs->lsr & 0x20)) {
	}
	regs->thr = (unsigned char)c;
	return c;
}

int __uart_getc(FILE *f)
{
	return 0;
}

static FILE __stdio = FDEV_SETUP_STREAM(__uart_putc, __uart_getc, NULL, _FDEV_SETUP_RW);

FILE *const stdin = &__stdio;
__strong_reference(stdin, stdout);
__strong_reference(stdin, stderr);
