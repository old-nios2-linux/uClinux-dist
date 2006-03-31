/*
** Copyright (C) 2002-2003 Analog Devices, Inc
 This file is subject to the terms and conditions of the GNU Library General
 Public License. See the file "COPYING.LIB" in the main directory of this
 archive for more details.

 Non-LGPL License also available as part of VisualDSP++
 http://www.analog.com/processors/resources/crosscore/visualDspDevSoftware.html

** unsigned long long multiplication:
** unsigned long long muli64(unsigned long long, unsigned long long)
*/

.text;
.align 2;
___mullu3:
    R3 = [SP+12];
    [--SP] = (R7:5);
    A1 = R0.L*R2.L (FU); 
	R5 = A1.W;
	R5 = R5 << 16;
	R7 = R5 >> 16;
	A1 = A1 >> 16; 
	A1 += R0.H * R2.L (FU);
	A1 += R0.L * R2.H (FU);
	R5 = A1.W;
	R5 = R5 << 16;
	A1 = A1 >> 16;
	R6 = R7 | R5;
	A1 += R3.L * R0.L (FU);
	A1 += R2.L * R1.L (FU);
	A1 += R0.H * R2.H (FU);
	R5 = A1.W;
	R5 = R5 << 16;
	R7 = R5 >> 16;
	A1 = A1 >> 16; 
	A1 += R1.H * R2.L (FU);
	A1 += R3.H * R0.L (FU);
	A1 += R1.L * R2.H (FU);
	A1 += R3.L * R0.H (FU);
	R5 = A1.W;
	R5 = R5 << 16;
	R1 = R7 | R5; 
	R0 = R6;
    (R7:5) = [SP++];
    RTS;
.___mullu3.end:
.global ___mullu3;
.type ___mullu3, STT_FUNC;
