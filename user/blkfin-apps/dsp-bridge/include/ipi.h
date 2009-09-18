/*
 * ipi.h
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __IPI_H
#define __IPI_H

void send_ipi0(void);
void clear_ipi0(void);
void send_ipi1(void);
void clear_ipi1(void);

#endif
