#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_
#include <icc.h>
#include "mempool.h"

struct coreb_icc_node {
	struct sm_icc_desc icc_info;
	struct gen_pool *pool;
};

extern struct coreb_icc_node coreb_info;

int iccqueue_getpending(sm_uint32_t srccpu);
int init_sm_session_table(void);
#endif
