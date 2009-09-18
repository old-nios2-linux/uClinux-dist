/*
 * User space application to demo dsp-bridge.
 *
 * Author:	Graff Yang
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "defs.h"
#include "message.h"
#include "demo_defs.h"

char *source_fn = "/etc/rc";
char *ringbf_fn = "dev/rbf0";
char *destina_fn = "/tmp/tmpfile";
char *coreb_fn = "/dev/dsp";
int coreb_fd;

int max_len;
int msg_num;

void *rbf_writer(void *data)
{
	int len_r;
	int len_w;
	int totallen;
	int source_fd;
	int ringbf_fd;
	struct stat sb;
	unsigned char *in_buf = data;

	source_fd = open(source_fn, O_RDONLY);

	if (source_fd < 0) {
		perror("unable to open source file\n");
		exit(1);
	}

	if (fstat(source_fd, &sb) < 0) {
		perror("fstat source file failed\n");
		exit(1);
	}	
	max_len = totallen = sb.st_size;

	ringbf_fd = open(ringbf_fn, O_WRONLY);
	if (ringbf_fd < 0) {
		perror("unable to open ringbuf file for write\n");
		exit(1);
	}

	while (totallen) {
		len_r = read(source_fd, in_buf, BUF_LEN);
		totallen -= len_r;
		len_w = write(ringbf_fd, in_buf, len_r);
		fprintf(stdout, "read in %03d, then write ringbuf %03d\n", len_r, len_w);
	}

	fprintf(stdout, "write ringbuf %d bytes finished\n", max_len);
	close(source_fd);
	close(ringbf_fd);
	return NULL;
}

void *rbf_reader(void *data)
{
	int len_r;
	int len_w;
	int totallen;
	int ringbf_fd;
	int destina_fd;
	unsigned char *out_buf = data;

	ringbf_fd = open(ringbf_fn, O_RDONLY | O_NONBLOCK);
	if (ringbf_fd < 0) {
		perror("unable to open ringbuf file for read\n");
		exit(1);
	}

	destina_fd = open(destina_fn, O_WRONLY | O_CREAT);
	if (destina_fd < 0) {
		perror("unable to open destination file\n");
		exit(1);
	}

	totallen = 0;
	fprintf(stdout, "file size: %03d\n", max_len);
	while (totallen < max_len) {
		len_r = read(ringbf_fd, out_buf, BUF_LEN);
		totallen += len_r;
		len_w = write(destina_fd, out_buf, len_r);
		fprintf(stdout, "read ringbuf %03d, then write destination %03d\n", len_r, len_w);
	}

	fprintf(stdout, "write destination file %d bytes finished\n", totallen);
	close(destina_fd);
	close(ringbf_fd);
	return NULL;
}

void user_msg_handler(int sig)
{
	struct message *msg;
	unsigned char *buf;

	if (sig != SIGUSR1) {
		fprintf(stderr, "unexpected signal\n");
		return;
	}

	while (1) {
		msg = NULL;
		ioctl(coreb_fd, CMD_COREB_RECEIVE_MESSAGE_DIRECT, &msg);
		if (!msg) break;

		switch (msg->type) {
	        case MSG_USER_1:
			buf = msg->data;
			buf[msg->size - 1] = '\0';
			fprintf(stdout, "user received MSG_USER_1: %s", buf);
			break;
		default:
			break;
		}
		ioctl(coreb_fd, CMD_COREB_PUT_RECEIVE_MESSAGE_SLOT, &msg);
		msg_num--;
	}
	return;
}

void int_handler(int sig)
{
	struct message *msg;

	fprintf(stdout, "Received SIGINT\n");

	ioctl(coreb_fd, CMD_COREB_GET_MESSAGE_SLOT, &msg);
	msg->type = MSG_DSP_RESET;
	ioctl(coreb_fd, CMD_COREB_SEND_MESSAGE_DIRECT, &msg);

	close(coreb_fd);
	exit(0);
}

int main(int argc, char *argv[])
{
	void *ret;
	int ringbf_fd;
	struct message *msg;
	struct message message;
	struct msg_quick_t *msgq;
	char in_buf[BUF_LEN];
	char out_buf[BUF_LEN];
	pthread_t th_a, th_b;

	if (argc > 1)
		source_fn = argv[1];

	fprintf(stdout, "source file is %s\n", source_fn);

	coreb_fd = open(coreb_fn, O_RDWR);
	if (coreb_fd < 0) {
		perror("unable to open CoreB control file\n");
		exit(1);
	}

	signal(SIGUSR1, user_msg_handler);
	signal(SIGINT, int_handler); 

	/* send message use pointer */
	ioctl(coreb_fd, CMD_COREB_GET_MESSAGE_SLOT, &msg);
	msg->type = MSG_DSP_START;
	ioctl(coreb_fd, CMD_COREB_SEND_MESSAGE_DIRECT, &msg);

	/*
	 * send message use message instance,
	 * kernel will handle the dcache flushes
	 */
	/* message.index = -1; */
	message.type = MSG_USER_1;
	message.pid = getpid();
	message.data = (unsigned char *)&msg_num;
	message.size = sizeof(unsigned int);
	msgq = (struct msg_quick_t *)message.data;
	/* let CoreB send 5 messages to this task */
	msgq->code = 5;
	ioctl(coreb_fd, CMD_COREB_SEND_MESSAGE, &message);
	
	ringbf_fd = open(ringbf_fn, O_RDWR);
	//flush(ringbf_fd, NULL);
	close(ringbf_fd);

	pthread_create(&th_a, NULL, rbf_writer, in_buf);
	pthread_create(&th_b, NULL, rbf_reader, out_buf);

	sleep(2);

	pthread_join(th_a, &ret);
	pthread_join(th_b, &ret);

	while (msg_num) {};

	ioctl(coreb_fd, CMD_COREB_GET_MESSAGE_SLOT, &msg);
	msg->type = MSG_DSP_RESET;
	ioctl(coreb_fd, CMD_COREB_SEND_MESSAGE_DIRECT, &msg);

	close(coreb_fd);
	return 0;
}

