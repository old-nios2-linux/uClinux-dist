/* Test: scl1
   Description: Tests scl_channel_send and scl_channel_recv calls on single node.  
   Note, that for scalar channels we only have blocking versions of send/recv.
   This test tests send/recv to several endpoints on the same node.  It tests
   all error conditions.  
*/

#include <mcapi.h>
#include <mcapi_test.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>

#define NUM_SIZES 4

#define DOMAIN 0

#define WRONG wrong(__LINE__);
void wrong(unsigned line)
{
}

mcapi_boolean_t send (mcapi_sclchan_send_hndl_t send_handle, mcapi_endpoint_t recv,unsigned long long data,uint32_t size,mcapi_status_t status,int exp_status) {
  mcapi_boolean_t rc = MCAPI_FALSE;
  switch (size) {
  case (8): mcapi_sclchan_send_uint8(send_handle,data,&status); break;
  case (16): mcapi_sclchan_send_uint16(send_handle,data,&status); break;
  case (32): mcapi_sclchan_send_uint32(send_handle,data,&status); break;
  case (64): mcapi_sclchan_send_uint64(send_handle,data,&status); break;
  default: coreb_msg(stderr,"ERROR: bad data size in call to send\n");
  };
  if (status == MCAPI_SUCCESS) {
    coreb_msg("endpoint=%i has sent %i byte(s): [%llu]\n",(int)send_handle,(int)size/8,data);
  }
  if (status == exp_status) {
    rc = MCAPI_TRUE;
  }
  return rc;
}

mcapi_boolean_t recv (mcapi_sclchan_recv_hndl_t recv_handle,uint32_t size,mcapi_status_t status,uint32_t exp_status,unsigned long long exp_data) {
  unsigned long long data = 0;
  mcapi_boolean_t rc = MCAPI_FALSE;
  uint64_t size_mask; 
  switch (size) {
  case (8): size_mask = 0xff;data=mcapi_sclchan_recv_uint8(recv_handle,&status); break;
  case (16): size_mask = 0xffff;data=mcapi_sclchan_recv_uint16(recv_handle,&status); break;
  case (32): size_mask = 0xffffffff;data=mcapi_sclchan_recv_uint32(recv_handle,&status); break;
  case (64): size_mask = 0xffffffffffffffffULL;data=mcapi_sclchan_recv_uint64(recv_handle,&status); break;
  default: coreb_msg("ERROR: bad data size in call to send\n");
  };
 
    coreb_msg("recv scalar[%llx] exp[%llx] %d %d\n",data, exp_data, status, exp_status);
  exp_data = exp_data & size_mask;
   
  if (status == exp_status) {
    rc = MCAPI_TRUE;
  }
  if (status == MCAPI_SUCCESS) {
    coreb_msg("endpoint=%i has received %i byte(s): [%llx]\n",(int)recv_handle,(int)size/8,data);
    if (data != exp_data) { 
       rc = MCAPI_FALSE; 
     }
  }
 
  return rc;
}

void recv_sclchan(mcapi_endpoint_t re,int size,mcapi_status_t status,int exp_status)
{
	mcapi_sclchan_recv_hndl_t r1;
	mcapi_request_t request;
	int rc;
	unsigned long long exp_data = 0x1122334455667788ULL;

	mcapi_sclchan_recv_open_i(&r1 /*recv_handle*/,re, &request, &status);
	rc = recv(r1, size, status, exp_status, exp_data);
	if (rc == MCAPI_FALSE)
		coreb_msg("scl recv wrong data\n");
	mcapi_sclchan_recv_close_i(r1,&request,&status); 

}

void icc_task_init(int argc, char *argv[])
{
	mcapi_status_t status;
	mcapi_request_t request;
	mcapi_param_t parms;
	mcapi_endpoint_t ep1,ep2,ep3;

	/* cases:
1: both named endpoints (1,2)
*/
	mcapi_sclchan_send_hndl_t s1;
	mcapi_sclchan_recv_hndl_t r1;
	mcapi_uint_t avail;
	int s;
	int i = 0;
	int sizes[NUM_SIZES] = {8,16,32,64};
	size_t size;
	mcapi_info_t version;
	mcapi_boolean_t rc = MCAPI_FALSE;
	uint64_t test_pattern = 0x1122334455667788ULL;

	/* create a node */
	mcapi_initialize(DOMAIN,SLAVE_NODE_NUM,NULL,&parms,&version,&status);
	if (status != MCAPI_SUCCESS) { WRONG }

	/* create endpoints */
	ep1 = mcapi_endpoint_create(SLAVE_PORT_NUM1,&status);
	if (status != MCAPI_SUCCESS) { WRONG }
	coreb_msg("mcapi sclchan test ep1 %x\n", ep1);

	ep2 = mcapi_endpoint_create(SLAVE_PORT_NUM2,&status);
	if (status != MCAPI_SUCCESS) { WRONG }
	coreb_msg("mcapi sclchan test ep2 %x\n", ep2);

	ep3 = mcapi_endpoint_get (DOMAIN,MASTER_NODE_NUM,MASTER_PORT_NUM2,MCA_INFINITE,&status);
	if (status != MCAPI_SUCCESS) { WRONG }

	coreb_msg("mcapi sclchan test ep3 %x\n", ep3);


	while (1) {
		if (icc_wait()) {
			recv_sclchan(ep1, sizes[i++],status,MCAPI_SUCCESS);
			if (i == NUM_SIZES) {
				mcapi_sclchan_connect_i(ep2,ep3,&request,&status);

				mcapi_sclchan_send_open_i(&s1,ep2, &request, &status);

				send(s1,ep3,test_pattern,64,status,MCAPI_SUCCESS);

				break;
			}
		}
	}

	mcapi_finalize(&status);

	coreb_msg("   Test PASSED\n");
	return;
}
