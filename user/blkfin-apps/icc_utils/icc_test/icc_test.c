/*
 * User space application to load a standalone Blackfin ELF
 * into the second core of a dual core Blackfin (like BF561).
 *
 * Copyright 2005-2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <bfin_sram.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <link.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>

#include <icc.h>

static bool force = false;

static int open_icc(void)
{
	int ret = open("/dev/icc", O_RDWR);
	if (ret < 0) {
		perror("unable to open /dev/icc");
		exit(10);
	}
	return ret;
}

static void send_test(int fd)
{
	struct sm_packet pkt;
	char buf[64] = "1234567890abcdef";
	memset(&pkt, 0, sizeof(struct sm_packet));

	pkt.local_ep = 9;
	pkt.remote_ep = 5;
	pkt.type = SP_PACKET;
	pkt.dst_cpu = 1;
	pkt.buf_len = 16;
	pkt.buf = buf;

	printf("begin create ep\n");
	ioctl(fd, CMD_SM_CREATE, &pkt);
	printf("finish create ep session index = %d\n", pkt.session_idx);


	ioctl(fd, CMD_SM_SEND, &pkt);

	ioctl(fd, CMD_SM_SHUTDOWN, &pkt);

}

static void recv_test(int fd)
{
	struct sm_packet pkt;
	char buf[64] = "1234567890abcdef";
	memset(&pkt, 0, sizeof(struct sm_packet));

	pkt.local_ep = 9;
	pkt.remote_ep = 5;
	pkt.type = SP_PACKET;
	pkt.dst_cpu = 1;
	pkt.buf_len = 16;
	pkt.buf = buf;

	printf("begin create ep\n");
	ioctl(fd, CMD_SM_CREATE, &pkt);
	printf("finish create ep session index = %d\n", pkt.session_idx);


	ioctl(fd, CMD_SM_SEND, &pkt);


	ioctl(fd, CMD_SM_RECV, &pkt);


	ioctl(fd, CMD_SM_SHUTDOWN, &pkt);
}
#define GETOPT_FLAGS "fshV"
#define a_argument required_argument
static struct option const long_opts[] = {
	{"force",      no_argument, NULL, 'f'},
	{"skip-start", no_argument, NULL, 's'},
	{"help",       no_argument, NULL, 'h'},
	{"version",    no_argument, NULL, 'V'},
	{NULL,         no_argument, NULL, 0x0}
};

__attribute__ ((noreturn))
static void show_version(void)
{
	exit(EXIT_SUCCESS);
}

__attribute__ ((noreturn))
static void show_usage(int exit_status)
{
	printf(
		"\nUsage: icc_test [options] \n"
		"\n"
		"Options:\n"
	);
	exit(exit_status);
}

int main(int argc, char *argv[])
{
	int i;
	struct stat stat;
	void *buf;
	int fd = open_icc();

	while ((i=getopt_long(argc, argv, GETOPT_FLAGS, long_opts, NULL)) != -1) {
		switch (i) {
		case 'f': force = true; break;
		case 's': force = true; break;
		case 'h': show_usage(EXIT_SUCCESS);
		case 'V': show_version();
		case ':':
			fprintf(stderr, "Option '%c' is missing parameter", optopt);
			show_usage(EXIT_FAILURE);
		case '?':
			fprintf(stderr, "Unknown option '%c' or argument missing", optopt);
			show_usage(EXIT_FAILURE);
		default:
			fprintf(stderr, "Unhandled option '%c'; please report this", i);
			return EXIT_FAILURE;
		}
	}

	send_test(fd);

	close(fd);

	return i;
}
