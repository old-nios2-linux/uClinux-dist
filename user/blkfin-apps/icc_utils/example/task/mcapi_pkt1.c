/* Test: msg1
   Description: Tests simple blocking msgsend and msgrecv calls to several endpoints
   on a single node 
*/

#include <mcapi.h>
#include <mcapi_test.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>

#define BUFF_SIZE 64

#define WRONG wrong(__LINE__);

#define DOMAIN 0


char buffer[BUFF_SIZE];

void wrong(unsigned line)
{
}

void recv_pktchan(mcapi_endpoint_t recv,mcapi_status_t status,int exp_status)
{
	size_t recv_size;
	mcapi_request_t request1;
	mcapi_request_t request2;
	mcapi_endpoint_t send_back;
	mcapi_pktchan_recv_hndl_t r1;
	void *pbuffer = NULL;

	mcapi_pktchan_recv_open_i(&r1,recv, &request1, &status);
	coreb_msg("open recv chan status %x   \n", status);

	mcapi_pktchan_recv_i(r1,(void **)&pbuffer,&request1,&status);
	if (status != exp_status) { WRONG}
	if (status == MCAPI_SUCCESS) {
		coreb_msg("endpoint=%i has received: [%s]\n",(int)recv,pbuffer);
	}
}

void icc_task_init(int argc, char *argv[])
{
  	mcapi_status_t status;
  	mcapi_info_t version;
  	mcapi_param_t parms;
	mcapi_endpoint_t ep1,ep2;
	int i;
	mcapi_uint_t avail;
	mcapi_request_t request;

	coreb_msg("[%s] %d\n", __func__, __LINE__);
	/* create a node */
  	mcapi_initialize(DOMAIN,SLAVE_NODE_NUM,NULL,&parms,&version,&status);
	if (status != MCAPI_SUCCESS) { WRONG }
	coreb_msg("[%s] %d\n", __func__, __LINE__);

	/* create endpoints */
  	ep1 = mcapi_endpoint_create(SLAVE_PORT_NUM1,&status);
	if (status != MCAPI_SUCCESS) { WRONG }
	coreb_msg("mcapi pktchan test ep1 %x   \n", ep1);

	while (1) {
		if (icc_wait()) {
			recv_pktchan(ep1,status,MCAPI_SUCCESS);
			break;
		}
	}

	mcapi_finalize(&status);
	coreb_msg("   Test PASSED\n");
	return;
}
