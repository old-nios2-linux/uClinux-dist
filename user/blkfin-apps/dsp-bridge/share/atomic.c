/*
 * File:		atomic.c
 * Based on:		arch/blackfin/mach-bf561/atomic.S
 * Author:		Graff Yang
 *
 * Description:		spinlock/unlock ops on non-cached memory
 * 
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/list.h>
#include "atomic.h"

void lock(unsigned long *lock)
{
asm(
	"p0 = %0;\n"
	"cli r0;\n"
".lock_retry:\n"
	"testset (p0);\n"
	"if cc jump .lock_done;\n"
	"cli r1;\n"
	"nop; nop;\n"
	"ssync;\n"
	"sti r1;\n"
	"jump .lock_retry;\n"
".lock_done:\n"
	"sti r0;\n"
	:
	: "r" (lock)
	: "R0", "R1", "P0"
   );
}

void unlock(unsigned long *lock)
{
asm(
	"p0 = %0;\n"
	"r0 = 0;\n"
	"cli r1;\n"
	"[p0] = r0;\n"
	"cli r0;\n"
	"nop; nop;\n"
	"ssync;\n"
	"sti r0;\n"
	"sti r1;\n"
	:
	: "r" (lock)
	: "R0", "R1", "P0"
   );
}

void asm_set(unsigned long *addr, int val)
{
asm(
	"p0 = r0;\n"
	"ssync;\n"
	"[p0] = r1;\n"
	"ssync;\n"
	:
	:
	: "P0"
   );
}

int asm_read(unsigned long *addr)
{
asm(
	"p0 = r0;\n"
	"ssync;\n"
	"r0 = [p0];\n"
	"ssync;\n"
	:
	:
	: "P0", "R0"
   );
}
