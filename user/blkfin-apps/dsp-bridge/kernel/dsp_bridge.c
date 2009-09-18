/*
 * Building a bridge between CoreA (Linux kernel) and CoreB (DSP) on a BF561
 *
 * Based on Bas Vermeulen <bvermeul@blackstar.xs4all.nl> coreb.c
 *
 * Copyright 2009 Analog Devices Inc.
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#define DEBUG

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <asm/cacheflush.h>
#include <asm/uaccess.h>
#include <asm/gpio.h>

#include "defs.h"
#include "ipi.h"
#include "message.h"
#include "ringbuf.h"

#define CTLDEV_NAME "dsp"
#define RBFDEV_NAME "rbf"

static struct ringbuf_dev {
	unsigned long channel;
	char name[DNAME_INLINE_LEN_MIN];
	struct miscdevice dev;
} ringbuf_devs[RBF_PAIR];

static struct kernel_msg_queue {
	spinlock_t lock;
	struct list_head queue_head;
	unsigned long qlen;
} kmsg_queue;

#ifdef CONFIG_COREB_LED_INDICATOR
static unsigned short leds[] = {
	GPIO_PF32, GPIO_PF33, GPIO_PF34, GPIO_PF35,
	GPIO_PF36, GPIO_PF37, GPIO_PF38, GPIO_PF39,
	GPIO_PF40, GPIO_PF41, GPIO_PF42, GPIO_PF43,
	GPIO_PF44, GPIO_PF45, GPIO_PF46, GPIO_PF47,
};
#endif

static struct mem_region {
	void *start, *end;
	int index;
} mem_regions[] = {
	{	/* L1 Scratchpad */
		.start = (void *)COREB_L1_SCRATCH_START,
		.end   = (void *)(COREB_L1_SCRATCH_START + L1_SCRATCH_LENGTH),
		.index = 0,
	},{	/* L1 Instruction SRAM/Cache */
		.start = (void *)COREB_L1_CODECACHE_START,
		.end   = (void *)(COREB_L1_CODECACHE_START + COREB_L1_CODECACHE_LENGTH),
		.index = 1,
	},{	/* L1 Instruction SRAM */
		.start = (void *)COREB_L1_CODE_START,
		.end   = (void *)(COREB_L1_CODE_START + COREB_L1_CODE_LENGTH),
		.index = 2,
	},{	/* L1 Data Bank B */
		.start = (void *)COREB_L1_DATA_B_START,
		.end   = (void *)(COREB_L1_DATA_B_START + COREB_L1_DATA_B_LENGTH),
		.index = 3,
	},{	/* L1 Data Bank A */
		.start = (void *)COREB_L1_DATA_A_START,
		.end   = (void *)(COREB_L1_DATA_A_START + COREB_L1_DATA_A_LENGTH),
		.index = 4,
	},{	/* L2 SRAM */
		.start = (void *)L2_START,
		.end   = (void *)(L2_START + L2_LENGTH),
		.index = 5,
	},{	/* SDRAM - just assume from 0 to top of ASYNC bank is OK */
		.start = (void*)0x00000000,
		.end   = (void*)0x30000000,
		.index = 6,
	}
};

static void dsp_start(void)
{
	bfin_write_SICA_SYSCR(bfin_read_SICA_SYSCR() & ~COREB_SRAM_INIT);
	CSYNC();
}

static void dsp_stop(void)
{
	send_ipi1();
	//bfin_write_SICA_SYSCR(bfin_read_SICA_SYSCR() | COREB_SRAM_INIT);
}

static void dsp_reset(void)
{
	static int num;
	struct message *msg;
	struct msg_quick_t *msgq;

	msg = get_message_slot();
	msg->type = MSG_DSP_RESET;
	msgq = (struct msg_quick_t *)msg->data;
	msgq->code = num++;
	send_message(msg);

	//send_ipi0();
}

static int dsp_get_mmregion_num(unsigned long arg)
{
	int num = ARRAY_SIZE(mem_regions);
	int __user *uarg = (void __user *) arg;

	if (put_user(num, uarg))
		return -EFAULT;

	return 0;
}

static int dsp_get_mmregion(unsigned long arg)
{
	int index;
	void __user *uarg = (void __user *) arg;
	struct mem_region mreg;
	if (copy_from_user(&mreg, uarg, sizeof(mreg)))
		return -EFAULT;

	index = mreg.index;
	if (index >= ARRAY_SIZE(mem_regions))
		memset(&mreg, 0, sizeof(mreg));
	else
		memcpy(&mreg, &mem_regions[index], sizeof(mreg));

	if (copy_to_user(uarg, &mreg, sizeof(mreg)))
		return -EFAULT;

	return 0;
}

static int dsp_get_message_slot(unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	struct message *msg;

	msg = get_message_slot();
	if (copy_to_user(uarg, &msg, sizeof(msg)))
		return -EFAULT;

	return 0;
}

static int dsp_put_receive_message_slot(unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	struct message *msg;

	if (copy_from_user(&msg, uarg, sizeof(msg)))
		return -EFAULT;

	put_receive_message_slot(msg->index);

	return 0;
}

static int dsp_send_message_direct(unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	struct message *msg;

	if (copy_from_user(&msg, uarg, sizeof(msg)))
		return -EFAULT;

	pr_debug("dsp_send_message_direct: user message from task %p\n", current);
	send_message(msg);

	return 0;
}

static int dsp_send_message(unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	struct message *msg;
	struct list_head list;
	unsigned long index;

	msg = get_message_slot();
	memcpy(&list, &msg->list, sizeof(msg->list));
	index = msg->index;

	if (copy_from_user(msg, uarg, sizeof(*msg)))
		return -EFAULT;

	memcpy(&msg->list, &list, sizeof(msg->list));
	msg->index = index;
	flush_dcache_range((unsigned long)msg->data, (unsigned long)msg->data + msg->size);

	pr_debug("dsp_send_message: user message from task %p\n", current);
	send_message(msg);

	return 0;
}

static int dsp_receive_message_direct(unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	struct message *msg = NULL;

	spin_lock(&kmsg_queue.lock);
	if (!list_empty(&kmsg_queue.queue_head)) {
		msg = list_entry(kmsg_queue.queue_head.next, typeof(*msg), list);
		list_del(&msg->list);
		kmsg_queue.qlen--;
	}
	spin_unlock(&kmsg_queue.lock);

	if (msg && copy_to_user(uarg, &msg, sizeof(msg)))
		return -EFAULT;

	return 0;
}

static int dsp_ctl_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case CMD_COREB_START:
		dsp_start();
		break;
	case CMD_COREB_STOP:
		dsp_stop();
		break;
	case CMD_COREB_RESET:
		dsp_reset();
		break;
	case CMD_COREB_GET_MMREGION_NUM:
		ret = dsp_get_mmregion_num(arg);
		break;
	case CMD_COREB_GET_MMREGION:
		ret = dsp_get_mmregion(arg);
		break;
	case CMD_COREB_GET_MESSAGE_SLOT:
		ret = dsp_get_message_slot(arg);
		break;
	case CMD_COREB_PUT_RECEIVE_MESSAGE_SLOT:
		ret = dsp_put_receive_message_slot(arg);
		break;
	case CMD_COREB_SEND_MESSAGE_DIRECT:
		ret = dsp_send_message_direct(arg);
		break;
	case CMD_COREB_SEND_MESSAGE:
		ret = dsp_send_message(arg);
		break;
	case CMD_COREB_RECEIVE_MESSAGE_DIRECT:
		ret = dsp_receive_message_direct(arg);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static ssize_t dsp_rbf_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long channel;
	channel = ((struct ringbuf_dev *)file->private_data)->channel;

	return rbf_read_user(channel, file->f_flags & O_NONBLOCK, buf, count);
}

static ssize_t dsp_rbf_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long channel;
	channel = ((struct ringbuf_dev *)file->private_data)->channel;

	return rbf_write_user(channel, file->f_flags & O_NONBLOCK, buf, count);
}

int dsp_rbf_open(struct inode *inode, struct file *file)
{
	unsigned long channel = 0;
	char *str = file->f_path.dentry->d_iname;

	strict_strtoul(str + strlen(RBFDEV_NAME), 10, &channel);

	file->private_data = &ringbuf_devs[channel];
	return 0;
}

int dsp_rbf_flush(struct file *file, fl_owner_t id)
{
	unsigned long channel;
	channel = ((struct ringbuf_dev *)file->private_data)->channel;

	/* ringbuf_flush_user(channel); */
	return 0;
}

int dsp_rbf_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t dsp_ctl_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t dsp_ctl_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

static struct file_operations dsp_rbf_fops = {
	.owner   = THIS_MODULE,
	.llseek  = NULL,
	.read    = dsp_rbf_read,
	.write   = dsp_rbf_write,
	.poll    = NULL,
	.ioctl   = NULL,
	.open    = dsp_rbf_open,
	.release = dsp_rbf_release,
	.flush   = dsp_rbf_flush,
};

static struct file_operations dsp_ctl_fops = {
	.owner   = THIS_MODULE,
	.llseek  = NULL,
	.read    = dsp_ctl_read,
	.write   = dsp_ctl_write,
	.poll    = NULL,
	.ioctl   = dsp_ctl_ioctl,
	.open    = NULL,
	.release = NULL,
};

static struct miscdevice dsp_ctl_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = CTLDEV_NAME,
	.fops  = &dsp_ctl_fops,
};

int sig_message(struct message *msg)
{
	struct siginfo info;
	struct task_struct *task;

	task = find_task_by_vpid(msg->pid);
	if (!task) {
		put_receive_message_slot(msg->index);
		pr_debug("can't send message to pid %ld\n", msg->pid);
		return 0;
	}

	spin_lock(&kmsg_queue.lock);
	list_add_tail(&msg->list, &kmsg_queue.queue_head);
	kmsg_queue.qlen++;
	spin_unlock(&kmsg_queue.lock);

	pr_debug("sig_message: sending dsp message to task %p\n", task);
	send_sig_info(SIGUSR1, &info, task);

	return 0;
}

static int __init dsp_bridge_init(void)
{
	int ret, i, j;
#ifdef CONFIG_COREB_LED_INDICATOR
	for (i = 0; i < ARRAY_SIZE(leds); i++) {
		ret = gpio_request(leds[i], "LED");
		if (ret) {
			for (j = i - 1; j >= 0; j--)
				gpio_free(leds[j]);
			pr_err("dsp_bridge: gpio request failed, \
				please turn off CONFIG_COREB_LED_INDICATOR.");
			goto gpio_fail;
		}
		gpio_direction_output(leds[i], 0);
	}
#endif
	ret = misc_register(&dsp_ctl_dev);
	if (ret)
		goto ctl_fail;

	message_init(sig_message);
	spin_lock_init(&kmsg_queue.lock);
	INIT_LIST_HEAD(&kmsg_queue.queue_head);
	kmsg_queue.qlen = 0;

	ringbuf_init();

	for (i = 0; i < RBF_PAIR; i++) {
		ringbuf_devs[i].channel = i;
		ringbuf_devs[i].dev.minor = MISC_DYNAMIC_MINOR;


		sprintf(ringbuf_devs[i].name, "%s%d", RBFDEV_NAME, i);
		ringbuf_devs[i].dev.name = ringbuf_devs[i].name; 
		ringbuf_devs[i].dev.fops  = &dsp_rbf_fops,

		pr_info("%s: creating ringbuf device %s\n", __func__, ringbuf_devs[i].dev.name);

		ret = misc_register(&ringbuf_devs[i].dev);
		if (ret) {
			for (j = i - 1; j >=0; j--)
				misc_deregister(&ringbuf_devs[j].dev);
			goto rbf_fail;
		}
	}
	return ret;

rbf_fail:
	misc_deregister(&dsp_ctl_dev);
ctl_fail:
#ifdef CONFIG_COREB_LED_INDICATOR
	for (i = 0; i < ARRAY_SIZE(leds); i++)
		gpio_free(leds[j]);
gpio_fail:
#endif
	return ret; 
}
module_init(dsp_bridge_init);

static void __exit dsp_bridge_exit(void)
{
	int i;

	for (i = 0; i < RBF_PAIR; i++) {
		misc_deregister(&ringbuf_devs[i].dev);
	}

	misc_deregister(&dsp_ctl_dev);
#ifdef CONFIG_COREB_LED_INDICATOR
	for (i = 0; i < ARRAY_SIZE(leds); i++)
		gpio_free(leds[i]);
#endif
}
module_exit(dsp_bridge_exit);

MODULE_AUTHOR("Graff Yang <graf.yang@analog.com>");
MODULE_DESCRIPTION("DSP bridge for BF561");
MODULE_LICENSE("GPL");
