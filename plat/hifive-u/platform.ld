STACK_SIZE = 256;   /* Stack size for a single core */
NHARTS = 4;	    /* Number of harts */
CODE_ALIGNMENT = 4; /* Align code on a 4 byte boundary */
DATA_ALIGNMENT = 8; /* Align data on a 8 byte boundary */

_clint = 0x2000000;
_mtime = 0x200bff8;
_mtimecmp = 0x2004000;
_uart = 0x10010000;
