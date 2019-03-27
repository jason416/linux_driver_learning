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

#include <linux/fs.h>	/* for file_ops */
#include <linux/cdev.h>	/* for cdev */

#define VSERIAL_MAJOR		256
#define VSERIAL_MINOR		0
#define VSERIAL_DEV_CNT		1
#define VSERIAL_DEV_NAME	"vserial"

/* cdev object */
static struct cdev vser_cdev;

/* file_ops */
static struct file_operations vser_ops = {
	.owner = THIS_MODULE,
};

static int __init vser_init(void)
{
	int ret;
	dev_t dev;
	
	dev = MKDEV(VSERIAL_MAJOR, VSERIAL_MINOR);
	ret = register_chrdev_region(dev, VSERIAL_DEV_CNT, VSERIAL_DEV_NAME);
	if(ret)
		goto reg_err;

	/* bind cdev object with file_ops */
	cdev_init(&vser_cdev, &vser_ops);
	vser_cdev.owner = THIS_MODULE;
	
	/* after cdev object initializaton over, add to cdev_map */
	ret = cdev_add(&vser_cdev, dev, VSERIAL_DEV_CNT);
	if(ret)
		goto add_err;

	return 0;

add_err:
	/* if add faild, unregister */
	unregister_chrdev_region(dev, VSERIAL_DEV_CNT);

reg_err:
	return ret;
}

static void __exit vser_exit(void)
{
	dev_t dev;
	
	dev = MKDEV(VSERIAL_MAJOR, VSERIAL_MINOR);

	/* delete cdev object, and unregister */
	cdev_del(&vser_cdev);
	unregister_chrdev_region(dev, VSERIAL_DEV_CNT);
}

module_init(vser_init);
module_exit(vser_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("jason416 <jason416@foxmail.com>");
MODULE_DESCRIPTION("A simple character device driver");
MODULE_ALIAS("virtual-serial");
