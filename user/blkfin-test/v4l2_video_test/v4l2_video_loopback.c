/*
 * Analog Devices video loopback test application
 *
 * Copyright (c) 2012 Analog Devices Inc.
 *
 * Author: Scott Jiang <Scott.Jiang.Linux@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/types.h>
#include <linux/videodev2.h>

struct cap_buffer {
	void *start;
	size_t length;
};

static const char *cap_dev = "/dev/video0";
static const char *lcd_dev = "/dev/fb0";
static int ignore = 0;
static int hflip = 0;
static int vflip = 0;
static int width = 800;
static int height = 480;

static void usage(const char *prog)
{
	printf("Usage: %s [-siHV]\n", prog);
	puts("  -s --size   capture image size (default wvga)\n"
	     "  -i --ignore ignore error frames\n"
	     "  -H --hflip  horizontally mirror\n"
	     "  -V --vflip  vertically mirror\n");
	exit(1);
}

static void parse_opts(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "size",    1, 0, 's' },
			{ "ignore",  0, 0, 'i' },
			{ "hflip",   1, 0, 'H' },
			{ "vflip",   1, 0, 'V' },
			{ NULL,      0, 0, 0   },
		};
		int c;

		c = getopt_long(argc, argv, "s:iHV", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 's':
			if (!strcmp(optarg, "qvga")) {
				width = 320;
				height = 240;
			} else if (!strcmp(optarg, "vga")) {
				width = 640;
				height = 480;
			}
			break;
		case 'i':
			ignore = 1;
			break;
		case 'H':
			hflip = 1;
			break;
		case 'V':
			vflip = 1;
			break;
		default:
			usage(argv[0]);
			break;
		}
	}
}

static int query_cap(int fd)
{
	struct v4l2_capability cap;
	int ret;

	ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		printf("ioctl VIDIOC_QUERYCAP error\n");
		return ret;
	}
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf("%s is not video capture device\n", cap_dev);
		return ret;
	}
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		printf("%s does not support streaming i/o\n", cap_dev);
		return ret;
	}
	return 0;
}

static int set_input(int fd)
{
	int input = 0;
	int ret;

	ret = ioctl(fd, VIDIOC_S_INPUT, &input);
	if (ret < 0)
		printf("ioctl VIDIOC_S_INPUT error\n");
	return ret;
}

static int set_fmt(int fd)
{
	struct v4l2_format fmt;
	int ret;

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = width;
	fmt.fmt.pix.height      = height;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
	fmt.fmt.pix.field       = V4L2_FIELD_ANY;

	ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
	if (ret < 0)
		printf("ioctl VIDIOC_S_FMT error\n");
	return ret;
}

static void set_control(int fd)
{
	struct v4l2_control flip;

	flip.id = V4L2_CID_HFLIP;
	flip.value = hflip;
	ioctl(fd, VIDIOC_S_CTRL, &flip);

	flip.id = V4L2_CID_VFLIP;
	flip.value = vflip;
	ioctl(fd, VIDIOC_S_CTRL, &flip);
}

int main(int argc, char *argv[])
{
	int cap_fd, lcd_fd, ret, i;
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;
	struct fb_var_screeninfo info;
	int lcd_size, cap_linelen, lcd_linelen;
	struct cap_buffer *pbuffer;
	unsigned char *lcd_buffer;

	parse_opts(argc, argv);

	cap_fd = open(cap_dev, O_RDWR);
	if (cap_fd < 0) {
		printf("cannot open '%s': %s\n", cap_dev, strerror(errno));
		return -1;
	}

	ret = query_cap(cap_fd);
	if (ret)
		goto err;

	ret = set_input(cap_fd);
	if (ret)
		goto err;


	ret = set_fmt(cap_fd);
	if (ret)
		goto err;

	set_control(cap_fd);

	req.count       = 3;
	req.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory      = V4L2_MEMORY_MMAP;
	ret = ioctl(cap_fd, VIDIOC_REQBUFS, &req);
	if (ret < 0) {
		if (EINVAL == errno)
			printf ("%s does not support memory mapping\n",
					cap_dev);
		else
			printf("ioctl VIDIOC_REQBUFS error\n");
		goto err;
	}
	printf("request %lu buffers\n", req.count);

	pbuffer = calloc(req.count, sizeof(*pbuffer));
	if (!pbuffer) {
		printf("no memory\n");
		goto err;
	}

	for (i = 0; i< req.count; i++) {
		memset(&buf, 0, sizeof(buf));
		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;
		ret = ioctl(cap_fd, VIDIOC_QUERYBUF, &buf);
		if (ret < 0) {
			printf("ioctl VIDIOC_QUERYBUF error\n");
			goto err1;
		}
		printf("bufffer[%d]: offset = %lu, lengeth = %lu\n",
				i, buf.m.offset, buf.length);
		pbuffer[i].length = buf.length;
		pbuffer[i].start = mmap(NULL, buf.length,
					PROT_READ | PROT_WRITE,
					MAP_SHARED,
					cap_fd, buf.m.offset);

		if (MAP_FAILED == pbuffer[i].start) {
			printf("mmap error: %s\n", strerror(errno));
			goto err1;
		}
		ret = ioctl(cap_fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			printf("ioctl VIDIOC_QBUF error\n");
			goto err1;
		}
	}

	lcd_fd = open(lcd_dev, O_RDWR);
	if (lcd_fd < 0) {
		printf("cannot open '%s': %s\n", lcd_dev, strerror(errno));
		goto err2;
	}

	ret = ioctl(lcd_fd, FBIOGET_VSCREENINFO, &info);
	if (ret < 0) {
		printf("ioctl FBIOGET_VSCREENINFO error\n");
		goto err3;
	}
	if (info.xres < width || info.yres < height) {
		printf("resolution mismatch: cap = %dx%d, lcd = %dx%d\n",
				width, height, info.xres, info.yres);
		goto err3;
	}

	lcd_size = info.xres * info.yres
			* info.bits_per_pixel / 8;
	lcd_buffer = mmap(NULL, lcd_size,
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				lcd_fd, 0);

	if (MAP_FAILED == lcd_buffer) {
		printf("lcd mmap error: %s\n", strerror(errno));
		goto err3;
	}
	memset(lcd_buffer, 0, lcd_size);
	cap_linelen = width * info.bits_per_pixel / 8;
	lcd_linelen = info.xres * info.bits_per_pixel / 8;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(cap_fd, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		printf("ioctl VIDIOC_STREAMON error\n");
		goto err4;
	}
	printf("video loopback start\n");

	while (1) {
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(cap_fd, VIDIOC_DQBUF, &buf);
		if (ret < 0) {
			printf("ioctl VIDIOC_DQBUF error\n");
			break;
		}

		if (!((buf.flags & V4L2_BUF_FLAG_ERROR) && ignore)) {
			for (i = 0; i < height; i++)
				memcpy(lcd_buffer + i * lcd_linelen,
				pbuffer[buf.index].start + i * cap_linelen,
				cap_linelen);
		}

		ret = ioctl(cap_fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			printf("ioctl VIDIOC_QBUF error\n");
			break;
		}
	}
	ioctl(cap_fd, VIDIOC_STREAMOFF, &type);
err4:
	munmap(lcd_buffer, lcd_size);
err3:
	close(lcd_fd);
err2:
	for (i = 0; i< req.count; i++)
		munmap(pbuffer[i].start, pbuffer[i].length);
err1:
	free(pbuffer);
err:
	close(cap_fd);
	return ret;
}
