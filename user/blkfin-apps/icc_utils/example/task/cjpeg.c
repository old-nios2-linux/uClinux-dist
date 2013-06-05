/*
 * cjpeg.c
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * Modified 2003-2008 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a command-line user interface for the JPEG compressor.
 * It should work on any system with Unix- or MS-DOS-style command lines.
 *
 * Two different command line styles are permitted, depending on the
 * compile-time switch TWO_FILE_COMMANDLINE:
 *	cjpeg [options]  inputfile outputfile
 *	cjpeg [options]  [inputfile]
 * In the second style, output is always to standard output, which you'd
 * normally redirect to a file or pipe to some other program.  Input is
 * either from a named file or from standard input (typically redirected).
 * The second style is convenient on Unix but is unhelpful on systems that
 * don't support pipes.  Also, you MUST use the first style if your system
 * doesn't do binary I/O to stdin/stdout.
 * To simplify script writing, the "-outfile" switch is provided.  The syntax
 *	cjpeg [options]  -outfile outputfile  inputfile
 * works regardless of which command line style is used.
 */

#include "jpeg-7/cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */
#include "jpeg-7/jversion.h"		/* for version message */

#ifdef USE_CCOMMAND		/* command-line reader for Macintosh */
#ifdef __MWERKS__
#include <SIOUX.h>              /* Metrowerks needs this */
#include <console.h>		/* ... and this */
#endif
#ifdef THINK_C
#include <console.h>		/* Think declares it here */
#endif
#endif

#include <mcapi.h>
#include <mcapi_test.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc */
#include <string.h>
#include <debug.h>

#define WRONG wrong(__LINE__);

#define DOMAIN 0

extern unsigned long jpg_pos;
extern unsigned long bmp_pos;
extern unsigned long jpg_size;
extern unsigned long bmp_buffer_len;

unsigned long *jpg_size_ptr;

static struct image_info {
  unsigned long bmp_buffer;
  unsigned long jpg_buffer;
  unsigned long bmp_len;
  unsigned long jpg_len;
  unsigned int cmd;
  unsigned int status;
};

static struct image_info *img_info;

/* Create the add-on message string table. */

#define JMESSAGE(code,string)	string ,


int cjpeg_main (int argc, char **argv);

static const char * const cdjpeg_message_table[] = {
#include "cderror.h"
  NULL
};

void wrong(unsigned line)
{
  COREB_DEBUG(1, "WRONG: line==%i \n",line);
}

extern void *get_free_buffer(unsigned long size);
extern void free_buffer(unsigned long addr, unsigned long size);


void send (mcapi_endpoint_t send, mcapi_endpoint_t recv, void* msg,int size, mcapi_status_t status,int exp_status) {
  int priority = 1;
  mcapi_request_t request;

  mcapi_msg_send_i(send,recv,msg,size,priority,&request,&status);
  if (status != exp_status) { WRONG}
  if (status == MCAPI_SUCCESS) {
  }
}

void recv_loopback (mcapi_endpoint_t recv,mcapi_status_t status,int exp_status) {
  size_t recv_size;
  mcapi_request_t request1;
  mcapi_request_t request2;
  mcapi_endpoint_t send_back;

	  mcapi_msg_recv_i(recv,img_info,sizeof(struct image_info),&request1,&status);
	  if (status != exp_status) { WRONG}
	  if (status == MCAPI_SUCCESS) {
	  }
	  
	  bmp_buffer_len = img_info->bmp_len;

	  int argc = 1;
	  char *argv[100];
	  *argv = "cjpeg";

	  cjpeg_main(argc, argv);

	  unsigned char *prt = (unsigned char*)(img_info->jpg_buffer);
	  
	  img_info->jpg_len = jpg_pos +1; 
	
	  send_back = mcapi_endpoint_get(DOMAIN,MASTER_NODE_NUM, MASTER_PORT_NUM1, MCA_INFINITE, &status);

	  mcapi_msg_send_i(recv,send_back,img_info,sizeof(struct image_info),1,&request2,&status);
	  if (status != exp_status) { WRONG}
}

void icc_task_init(int argc, char *argv[]) {
  mcapi_status_t status;
  mcapi_info_t version;
  mcapi_param_t parms;
  mcapi_endpoint_t ep1,ep2;
  mcapi_uint_t avail;

  /* create a node */
  mcapi_initialize(DOMAIN,SLAVE_NODE_NUM,NULL,&parms,&version,&status);

  if (status != MCAPI_SUCCESS) { WRONG }

  /* create endpoints */
  ep1 = mcapi_endpoint_create(SLAVE_PORT_NUM1,&status);
  if (status != MCAPI_SUCCESS) { WRONG }
  /* send and recv messages on the endpoints */

  while(1) {
	if (icc_wait()) {
		recv_loopback(ep1,status,MCAPI_SUCCESS);
		if (status != MCAPI_SUCCESS) { WRONG }
	}
  }

  mcapi_endpoint_delete(ep1,&status);

  mcapi_finalize(&status);

  return; 
}

//static boolean is_targa;	/* records user -targa switch */



LOCAL(cjpeg_source_ptr)
select_file_type (j_compress_ptr cinfo, FILE * infile)
{
    return jinit_read_bmp(cinfo);//added by aaron
			/* suppress compiler warnings */
}


/*
 * Argument-parsing code.
 * The switch parser is designed to be useful with DOS-style command line
 * syntax, ie, intermixed switches and file names, where only the switches
 * to the left of a given file name affect processing of that file.
 * The main program in this file doesn't actually use this capability...
 */


static const char * progname;	/* program name for error messages */
static char * outfilename;	/* for -outfile switch */

static FILE test_input_file; 
static FILE test_output_file; 

/*
 * The main program.
 */

int
cjpeg_main (int argc, char **argv)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
#ifdef PROGRESS_REPORT
  struct cdjpeg_progress_mgr progress;
#endif
  cjpeg_source_ptr src_mgr;
  FILE * input_file;
  FILE * output_file;
  JDIMENSION num_scanlines;
  /* On Mac, fetch a command line. */
#ifdef USE_CCOMMAND
  argc = ccommand(&argv);
#endif

  img_info->status = 0;

  progname = argv[0];

  if (progname == NULL || progname[0] == 0)
    progname = "cjpeg";		/* in case C library doesn't provide it */

  /* Initialize the JPEG compression object with default error handling. */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  /* Add some application-specific error messages (from cderror.h) */
  jerr.addon_message_table = cdjpeg_message_table;
  jerr.first_addon_message = JMSG_FIRSTADDONCODE;
  jerr.last_addon_message = JMSG_LASTADDONCODE;

  /* Now safe to enable signal catcher. */
#ifdef NEED_SIGNAL_CATCHER
  enable_signal_catcher((j_common_ptr) &cinfo);
#endif

  /* Initialize JPEG parameters.
   * Much of this may be overridden later.
   * In particular, we don't yet know the input file's color space,
   * but we need to provide some value for jpeg_set_defaults() to work.
   */

  cinfo.in_color_space = JCS_RGB; /* arbitrary guess */
  jpeg_set_defaults(&cinfo);

  /* Scan command line to find file names.
   * It is convenient to use just one switch-parsing routine, but the switch
   * values read here are ignored; we will rescan the switches after opening
   * the input file.
   */

  input_file = img_info->bmp_buffer;
  output_file = img_info->jpg_buffer;

  /* Figure out the input file format, and set up to read it. */
  src_mgr = select_file_type(&cinfo, input_file);
  src_mgr->input_file = input_file;

  /* Read the input file header to obtain file size & colorspace. */
  (*src_mgr->start_input) (&cinfo, src_mgr);

  /* Now that we know input colorspace, fix colorspace-dependent defaults */
  jpeg_default_colorspace(&cinfo);

  /* Adjust default compression parameters by re-parsing the options */

  /* Specify data destination for compression */
  jpeg_stdio_dest(&cinfo, output_file);

  /* Start compressor */
  jpeg_start_compress(&cinfo, TRUE);

  /* Process data */
  while (cinfo.next_scanline < cinfo.image_height) {
    num_scanlines = (*src_mgr->get_pixel_rows) (&cinfo, src_mgr);
    (void) jpeg_write_scanlines(&cinfo, src_mgr->buffer, num_scanlines);
  }
  /* Finish compression and release memory */
  (*src_mgr->finish_input) (&cinfo, src_mgr);
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  /* All done. */
  img_info->status = 1;
  img_info->jpg_len = jpg_pos + 1;//going to be used in bmp2jpg.c for final jpg output

  return 0;			/* suppress no-return-value warnings */
}
