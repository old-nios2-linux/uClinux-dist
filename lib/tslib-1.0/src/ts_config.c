/*
 *  tslib/src/ts_config.c
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 * $Id: ts_config.c,v 1.5 2004/10/19 22:01:27 dlowder Exp $
 *
 * Read the configuration and load the appropriate drivers.
 */
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tslib-private.h"

struct module_list {
  TSAPI struct tslib_module_info * (*mod_init)(struct tsdev *dev, const char *params);
	char *params;
};

/* available modules */
TSAPI struct tslib_module_info *input_mod_init(struct tsdev *dev, const char *params);
TSAPI struct tslib_module_info *pthres_mod_init(struct tsdev *dev, const char *params);
TSAPI struct tslib_module_info *variance_mod_init(struct tsdev *dev, const char *params);
TSAPI struct tslib_module_info *linear_mod_init(struct tsdev *dev, const char *params);

/* Put your own list here, to replace ts.conf */
/* first one must be raw */
static struct module_list config_list[] = {
	{ input_mod_init, NULL },
	{ pthres_mod_init, "pmin=1" },
	{ variance_mod_init, "delta=30" },
	{ linear_mod_init, NULL },
};

#define NR_CONF (sizeof(config_list) / sizeof(config_list[0]))

int ts_config(struct tsdev *ts)
{
	int ret = 0;
	int i;

	for (i = 0; i< NR_CONF; i++) {
		struct tslib_module_info *info;
		info = config_list[i].mod_init(ts, config_list[i].params); 
		if (i == 0) {
			ret = __ts_attach_raw(ts, info);
		} else {
			ret = __ts_attach(ts, info);
		}
		if (ret != 0) {
			ts_error("Couldnt load module %d\n", i);
			break;
		}
	}

	return ret;
}
