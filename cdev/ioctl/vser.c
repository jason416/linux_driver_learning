/*************************************************************************
*    > File Name: vser.c
*    > Author: jason416
*    > Mail: jason416@foxmail.com 
*    > Created Time: Sat 30 Mar 2019 10:31:46 AM CST
*    > License: GPL v2
*************************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>

#include <linux/ioctl.h>
#include <linux/uaccess.h>

#include "vser.h"

#define VSER_MAJOR		300
#define VSER_MINOR		0
#define VSER_DEV_CNT	1
#define VSER_DEV_NAME	"vser"

#define KFIFO_SIZE		32

struct vser_dev {
	unsigned int baud;
	struct option opt;
	struct cdev cdev;
};

/* driver data */
struct vser_dev vsdev;
DEFINE_KFIFO(vsfifo, char, 32);

static int vser_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int vser_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t vser_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	int ret;
	ssize_t copied = 0;

	ret = kfifo_to_user(&vsfifo, buf, count, &copied);

	return ret == 0 ? copied : ret;
}

static ssize_t vser_write(struct file *filp, const char __user *buf, size_t count, loff_t *ops)
{
	int ret;
	ssize_t copied = 0;

	return (ret = kfifo_from_user(&vsfifo, buf, count, &copied)) ? ret : copied;
}

static long vser_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if(_IOC_TYPE(cmd) != VS_MAGIC)
		return -ENOTTY;

	switch(cmd) {
	case VS_SET_BAUD:
		vsdev.baud = arg;
		break;
	case VS_GET_BAUD:
		arg = vsdev.baud;
		break;
	case VS_SET_FFMT:
		/* on success, return 0; on error, return num copied successfully in bytes */
		if(copy_from_user(&vsdev.opt, (struct option __user *)arg, sizeof(struct option)))
			return -EFAULT;
		break;
	case VS_GET_FFMT:
		/* on success, return 0; on error, return num copied successfully in bytes */
		if(copy_to_user((struct option __user *)arg, &vsdev.opt, sizeof(struct option)))
			return -EFAULT;
		break;
	default:
		return -ENOTTY;
	}
	
	return 0;
}

static struct file_operations vser_ops = {
	.owner = THIS_MODULE,
	.open = vser_open,
	.release = vser_release,
	.read = vser_read,
	.write = vser_write,
	.unlocked_ioctl = vser_ioctl,
};

/**************** module_int and module_exit **************/

static int __init vser_init(void) 
{
	int ret;
	dev_t dev;

	dev = MKDEV(VSER_MAJOR, VSER_MINOR);
	ret = register_chrdev_region(dev, VSER_DEV_CNT, VSER_DEV_NAME);
	if(ret)
		goto reg_err;

	cdev_init(&vsdev.cdev, &vser_ops);
	vsdev.cdev.owner = THIS_MODULE;
	
	/* initialize defualt value */
	vsdev.baud = 115200;
	vsdev.opt.data_bit = 8;
	vsdev.opt.parity = none_parity;
	vsdev.opt.stop_bit = 1;

	ret = cdev_add(&vsdev.cdev, dev, VSER_DEV_CNT);
	if(ret)
		goto add_err;

	/* go here, means everythig is ok, return */
	return 0;

add_err:
	printk("add device error!");
	unregister_chrdev_region(dev, VSER_DEV_CNT);
reg_err:
	printk("register error!\n");
	return ret;
}

static void __exit vser_exit(void)
{
	dev_t dev;

	dev = MKDEV(VSER_MAJOR, VSER_MINOR);

	cdev_del(&vsdev.cdev);
	unregister_chrdev_region(dev, VSER_DEV_CNT);
}

module_init(vser_init);
module_exit(vser_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("jason416 <jason416@foxmail.com>");
MODULE_DESCRIPTION("A simple character device driver");
MODULE_ALIAS("virtual-serial");

