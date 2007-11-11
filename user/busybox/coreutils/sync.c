/* vi: set sw=4 ts=4: */
/*
 * Mini sync implementation for busybox
 *
 * Copyright (C) 1995, 1996 by Bruce Perens <bruce@pixar.com>.
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

/* BB_AUDIT SUSv3 N/A -- Matches GNU behavior. */

#include <stdlib.h>
#include <unistd.h>
#ifdef EMBED
#include <config/autoconf.h>
#include <signal.h>
#endif
#include "busybox.h"

int sync_main(int argc, char **argv);
int sync_main(int argc, char **argv)
{
#ifdef CONFIG_USER_FLATFSD_FLATFSD
	int   verbose = 0;
	int   flash = 0;
	int   opt;

	while ((opt=getopt(argc, argv, "vf")) != -1) {
		switch(opt) {
			case 'v':
				verbose = 1;
				break;
			case 'f':
				flash = 1;
				break;
			default:
				bb_show_usage();
		}
	}

	/* get the pid of flatfsd */
	if (flash) {
		if (verbose)
			puts("sync: flash");
		system("exec flatfsd -s");
	}
	if (verbose)
		puts("sync: file systems");
#else
	bb_warn_ignoring_args(argc - 1);
#endif

	sync();

	return EXIT_SUCCESS;
}
