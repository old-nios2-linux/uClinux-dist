/*
 * atomic.h
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __ATOMIC_H
#define __ATOMIC_H

void lock(unsigned long *lock);
void unlock(unsigned long *lock);

void asm_set(unsigned long *addr, int val);
int asm_read(unsigned long *addr);

#endif
