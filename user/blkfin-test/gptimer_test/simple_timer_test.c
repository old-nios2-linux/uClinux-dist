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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
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

#include <asm/bfin_simple_timer.h>

char *timer_dev = "/dev/timer0";

static int timer_test(int fd, unsigned long period, unsigned long width, unsigned long mode)
{
	ioctl(fd, BFIN_SIMPLE_TIMER_SET_PERIOD, period);

	ioctl(fd, BFIN_SIMPLE_TIMER_SET_WIDTH, width);

	ioctl(fd, BFIN_SIMPLE_TIMER_SET_MODE, mode);

	ioctl(fd, BFIN_SIMPLE_TIMER_START, 0);

	return 0;
}

#define GETOPT_FLAGS "d:p:w:m:sthV"
#define a_argument required_argument
static struct option const long_opts[] = {
	{"timer_dev",	no_argument, NULL, 'd'},
	{"period",	no_argument, NULL, 'p'},
	{"width",	no_argument, NULL, 'w'},
	{"mode",	no_argument, NULL, 'm'},
	{"start",	no_argument, NULL, 's'},
	{"stop",	no_argument, NULL, 't'},
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
		"\nUsage: simple_timer_test [options] \n"
		"\n"
		"Options:\n"
		"-p period\n"
		"-w width\n"
		"-m mode\n"
		"-s start timer\n"
		"-t stop_timer\n"
	);
	exit(exit_status);
}

int main(int argc, char *argv[])
{
	int i;
	int timer_fd;
	unsigned long period = 1000000;
	unsigned long width = (period >> 1);
	int mode = 1;

	while ((i=getopt_long(argc, argv, GETOPT_FLAGS, long_opts, NULL)) != -1) {
		switch (i) {
		case 'd':
			timer_dev = optarg;
			break;
		case 'p':
			period = atol(optarg);
			break;
		case 'w':
			width = atol(optarg);
			break;
		case 'm':
			mode = atol(optarg);
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

	timer_fd = open(timer_dev, O_RDWR);
	if (timer_fd < 0) {
		perror("unable to open timer dev\n");
		exit(10);
	}

	timer_test(timer_fd, period, width, mode);

	sleep(2);

	close(timer_fd);

	return 0;
}
