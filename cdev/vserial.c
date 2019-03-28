/*************************************************************************
*    > File Name: vserial.c
*    > Author: jason416
*    > Mail: jason416@foxmail.com 
*    > Created Time: Wed 27 Mar 2019 10:51:41 PM CST
*    > License: GPL v2
*************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#include <linux/fs.h>		/* for file_ops */
#include <linux/cdev.h>		/* for cdev */
#include <linux/kfifo.h>	/* for kfifo */

#define VSERIAL_MAJOR		256
#define VSERIAL_MINOR		0
#define VSERIAL_DEV_CNT		2
#define VSERIAL_DEV_NAME	"vserial"
#define KFIFO_SIZE			32

/* new class, derived from cdev class */
struct vser_dev {
	struct kfifo *fifo;		/* private data */
	struct cdev cdev;		/* base class */
};

static struct vser_dev vser_dev[VSERIAL_DEV_CNT];

/* define two kfifo object, which has 32 elements, type char; each one for a vser_dev*/
static DEFINE_KFIFO(vser_fifo0, char, KFIFO_SIZE);
static DEFINE_KFIFO(vser_fifo1, char, KFIFO_SIZE);

/********** define operations for VFS ************/
static int vser_open(struct inode *inode, struct file *filp)
{
	/* inode will record this dev's cdev, like inode::i_cdev -> vser_dev::cdev,
	 * each devices has diffrent cdev struct.
	 */
	filp->private_data = container_of(inode->i_cdev, struct vser_dev, cdev);
	return 0;
}

static int vser_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t vser_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	int ret = 0;
	ssize_t copied = 0;

	/* get this device's object addr */
	struct vser_dev *dev = filp->private_data;
	
	/* copy kfifo's content to user space */
	ret = kfifo_to_user(dev->fifo, buf, count > KFIFO_SIZE ? KFIFO_SIZE : count, &copied);
	if(ret != 0)
		return ret;
	else
		return copied;
}

static ssize_t vser_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
	int ret = 0;
	ssize_t copied = 0;

	/* get this device's object addr */
	struct vser_dev *dev = filp->private_data;

	/* copy user space's content to kfifo */
	ret = kfifo_from_user(dev->fifo, buf, count > KFIFO_SIZE ? KFIFO_SIZE : count, &copied);
	if(ret != 0)
		return ret;
	else
		return copied;
}

/* file_ops, initialize with functions above */
static struct file_operations vser_ops = {
	.owner = THIS_MODULE,
	.open  = vser_open,
	.release = vser_release,
	.read  = vser_read,
	.write = vser_write, 
};

static int __init vser_init(void)
{
	int i, ret;
	dev_t dev;
	
	/* make a dev_t, and register */
	dev = MKDEV(VSERIAL_MAJOR, VSERIAL_MINOR);
	ret = register_chrdev_region(dev, VSERIAL_DEV_CNT, VSERIAL_DEV_NAME);
	if(ret)
		goto reg_err;

	/* bind cdev object with file_ops, then add to cdev_map */
	for(i = 0; i < VSERIAL_DEV_CNT; i++) {
		cdev_init(&vser_dev[i].cdev, &vser_ops);
		vser_dev[i].cdev.owner = THIS_MODULE;
		vser_dev[i].fifo = i ? (struct kfifo *)&vser_fifo1 : (struct kfifo *)&vser_fifo0; /* compatible pointer, -> start addr of struct */

		/* each device has a cdev object */
		ret = cdev_add(&vser_dev[i].cdev, dev + i, 1);
		if(ret)
			goto add_err;
	}
	
	return 0;

add_err:
	/* if add faild, del cdev from cdev_map, then unregister */
	for(--i ; i >= 0; i--)
		cdev_del(&vser_dev[i].cdev);
	unregister_chrdev_region(dev, VSERIAL_DEV_CNT);

reg_err:
	return ret;
}

static void __exit vser_exit(void)
{
	int i;
	dev_t dev;
	
	dev = MKDEV(VSERIAL_MAJOR, VSERIAL_MINOR);

	/* delete cdev object, and unregister */
	for(i = 0; i < VSERIAL_DEV_CNT; i++)
		cdev_del(&vser_dev[i].cdev);
	unregister_chrdev_region(dev, VSERIAL_DEV_CNT);
}

module_init(vser_init);
module_exit(vser_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("jason416 <jason416@foxmail.com>");
MODULE_DESCRIPTION("A simple character device driver");
MODULE_ALIAS("virtual-serial");
