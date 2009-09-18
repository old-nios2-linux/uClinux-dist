/*
 * File:		ipi.c
 * Based on:
 * Author:		Graff Yang
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <mach/cdefBF561.h>

#ifdef __DSP__
/* set CA_supplement_int0 */
void send_ipi0(void)
{
	SSYNC();
	bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << 6));
	SSYNC();
}

/* clear CB_supplement_int0 */
void clear_ipi0(void)
{
	SSYNC();
	bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << 11));
	SSYNC();
}
/* set CA_supplement_int1 */
void send_ipi1(void)
{
	SSYNC();
	bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << 8));
	SSYNC();
}

/* clear CB_supplement_int1 */
void clear_ipi1(void)
{
	SSYNC();
	bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << 13));
	SSYNC();
}
#else

/* set CB_supplement_int0 */
void send_ipi0(void)
{
	SSYNC();
	bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << 7));
	SSYNC();
}

/* clear CA_supplement_int0 */
void clear_ipi0(void)
{
	SSYNC();
	bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << 10));
	SSYNC();
}
/* set CB_supplement_int1 */
void send_ipi1(void)
{
	SSYNC();
	bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << 9));
	SSYNC();
}

/* clear CA_supplement_int1 */
void clear_ipi1(void)
{
	SSYNC();
	bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << 12));
	SSYNC();
}
#endif
