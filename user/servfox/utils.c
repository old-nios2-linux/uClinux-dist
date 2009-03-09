/****************************************************************************
#	 	Spcaview:  Spca5xx Grabber                                  #
# 		Copyright (C) 2004 2005 Michel Xhaard                       #
#                                                                           #
# This program is free software; you can redistribute it and/or modify      #
# it under the terms of the GNU General Public License as published by      #
# the Free Software Foundation; either version 2 of the License, or         #
# (at your option) any later version.                                       #
#                                                                           #
# This program is distributed in the hope that it will be useful,           #
# but WITHOUT ANY WARRANTY; without even the implied warranty of            #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             #
# GNU General Public License for more details.                              #
#                                                                           #
# You should have received a copy of the GNU General Public License         #
# along with this program; if not, write to the Free Software               #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA #
#                                                                           #
****************************************************************************/

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/types.h>
#include <string.h>
#include <fcntl.h>
#include <wait.h>
#include <sys/time.h>
#include <limits.h>

void exit_fatal(char *messages)
{
	printf("%s \n",messages);
	exit(1);
}

double
ms_time (void)
{
  static struct timeval tod;
  gettimeofday (&tod, NULL);
  return ((double) tod.tv_sec * 1000.0 + (double) tod.tv_usec / 1000.0);

}

int
get_jpegsize (unsigned char *buf, int insize)
{
 int i; 	
 for ( i= 1024 ; i< insize; i++) {
 	if ((buf[i] == 0xFF) && (buf[i+1] == 0xD9)) return i+10;
 }
 return -1;
}
