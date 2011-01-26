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

	printf("sp packet %d\n", pkt.type);

	printf("begin create ep\n");
	ioctl(fd, CMD_SM_CREATE, &pkt);
	printf("finish create ep session index = %d\n", pkt.session_idx);


	ioctl(fd, CMD_SM_SEND, &pkt);

	memset(buf, 0, 64);
	ioctl(fd, CMD_SM_RECV, &pkt);

	printf("%s \n", pkt.buf);

	sleep(3);
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

static void exec_task(int fd)
{
	struct sm_packet pkt;
	struct sm_task *task1 = NULL;
	int taskargs = 2;
	char taskargv0[] = "task1";
	char taskargv1[] = "icc";
	int packetsize = sizeof(struct sm_task) + MAX_TASK_NAME * (taskargs - 1);

	task1 = malloc(packetsize);
	if (!task1) {
		printf("malloc failed\n");
	}
	memset(task1, 0, packetsize);
	task1->task_init = 0xFEB0C000;
	task1->task_exit = 0;
	task1->task_argc = 2;
	strcpy(task1->task_argv[0], taskargv0);
	strcpy(task1->task_argv[1], taskargv1);

	memset(&pkt, 0, sizeof(struct sm_packet));
	pkt.local_ep = 0;
	pkt.remote_ep = 5;
	pkt.type = SP_TASK_MANAGER;
	pkt.dst_cpu = 1;
	pkt.buf_len = packetsize;
	pkt.buf = task1;

	ioctl(fd, CMD_SM_CREATE, &pkt);

	ioctl(fd, CMD_SM_SEND, &pkt);

//	ioctl(fd, CMD_SM_SHUTDOWN, &pkt);

//	free(task1);
}
#define GETOPT_FLAGS "rsekhV"
#define a_argument required_argument
static struct option const long_opts[] = {
	{"receive",	no_argument, NULL, 'r'},
	{"send",	no_argument, NULL, 's'},
	{"exec",	no_argument, NULL, 'e'},
	{"kill",	no_argument, NULL, 'k'},
	{"help",	no_argument, NULL, 'h'},
	{"version",	no_argument, NULL, 'V'},
	{NULL,		no_argument, NULL, 0x0}
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
		case 'r':
			recv_test(fd);
			break;
		case 's':
			send_test(fd);
			break;
		case 'e':
			exec_task(fd);
			break;
		case 'k':
			break;
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

	close(fd);

	return i;
}
