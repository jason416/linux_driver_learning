/*************************************************************************
*    > File Name: led_driver.c
*    > Author: jason416
*    > Mail: jason416@foxmail.com 
*    > Created Time: Sun 21 Apr 2019 06:16:15 AM CST
*    > License: GPL v2
*************************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>

#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>

#include "led.h"

#ifdef DEBUG
#define DEBUG_INFO(fmt, arg...) printk(fmt, ##arg)
#else
#define DEBUG_INFO(fmt, arg...)
#endif

#define LED_MAJOR	  256
#define LED_DEV_NAME "led"

struct led_dev {
	unsigned int __iomem *con;
	unsigned int __iomem *dat;
	unsigned int pin;
	atomic_t available;
	struct cdev cdev;
};

static int led_open(struct inode *inode, struct file *filp)
{
	struct led_dev *led = container_of(inode->i_cdev, struct led_dev, cdev);

	filp->private_data = led;	/* each led_dev has its private data */
	if(atomic_dec_and_test(&led->available)) /* try get this device */
		return 0;
	else {
		atomic_inc(&led->available);
		return -EBUSY;
	}
}

static int led_release(struct inode *inode, struct file *filp)
{
	struct led_dev *led = container_of(inode->i_cdev, struct led_dev, cdev);
	
	writel(readl(led->dat) & ~(0x1 << led->pin), led->dat); /* turn off */

	atomic_inc(&led->available); /* set available */
	return 0;
}

static long led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct led_dev *led = filp->private_data;	

	if(_IOC_TYPE(cmd) != LED_MAGIC)
		return -ENOTTY;

	switch(cmd) {
		case LED_ON:
			DEBUG_INFO("cmd = LED_ON! pin = %d\n", led->pin);
			writel(readl(led->dat) | (0x1 << led->pin), led->dat);
			break;
		case LED_OFF:
			DEBUG_INFO("cmd = LED_OFF! pin = %d\n", led->pin);
			writel(readl(led->dat) & ~(0x1 << led->pin), led->dat);
			break;
		default:
			return -ENOTTY;
	}

	return 0;
}

static struct file_operations led_opts = {
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_release,
	.unlocked_ioctl = led_ioctl,
};

static int led_probe(struct platform_device *pdev)
{
	int ret;
	dev_t dev;
	struct led_dev *led;
	struct resource *res;
	unsigned int pin;

	/* get each platform_device private data(pin here) */
	pin = *(unsigned int *)pdev->dev.platform_data;
	DEBUG_INFO("dev_name: %s, pin = %d\n", pdev->name, pin);

	dev = MKDEV(LED_MAJOR, pdev->id);
	ret = register_chrdev_region(dev, 1, LED_DEV_NAME);
	if(ret)
		goto reg_err;

	/* allocate memory for struct led_dev */
	led = kzalloc(sizeof(struct led_dev), GFP_KERNEL);
	if(!led) {
		ret = -ENOMEM;
		goto mem_err;
	}

	cdev_init(&led->cdev, &led_opts);
	led->cdev.owner = THIS_MODULE;
	ret = cdev_add(&led->cdev, dev, 1);
	if(ret)
		goto add_err;

	/* get resource from platform_device */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		ret = -ENOENT;
		goto res_err;
	}

	/* io remap, kernel can not access physic addr directly */
	DEBUG_INFO("res->start = 0x%x\n", res->start);
	led->con = ioremap(res->start, resource_size(res));
	if(!led->con) {
		ret = -EBUSY;
		goto map_err;
	}
	led->dat = led->con + 1; /* unsigned int * + 1, offset 32bits */
	led->pin = pin;
	atomic_set(&led->available, 1);	/* set available */

	DEBUG_INFO("con base = 0x%x\n", led->con);
	/* set gpio CONx as 1(Output mode), and set pin0 as 1(light off) */
	writel((readl(led->con) & ~(0xf << 4 * led->pin)) | (0x1 << 4 * led->pin), led->con);
	writel(readl(led->dat) & ~(0x1 << led->pin), led->dat);

	/* praper data for matched device */
	platform_set_drvdata(pdev, led);

	return 0;

map_err:
res_err:
	cdev_del(&led->cdev);
add_err:
	kfree(led);
mem_err:
	unregister_chrdev_region(dev, 1);
reg_err:
	return ret;
}

static int led_remove(struct platform_device *pdev)
{
	dev_t dev;
	struct led_dev *led; 

	/* get data from matched device for driver */
	led = platform_get_drvdata(pdev);
	dev = MKDEV(LED_MAJOR, pdev->id);

	iounmap(led->con);
	cdev_del(&led->cdev);
	kfree(led);
	unregister_chrdev_region(dev, 1);

	return 0;
}

struct platform_driver led_drv = {
	.driver = {
		.name = LED_DEV_NAME,	/* must match to its platform_device name */
		.owner = THIS_MODULE,
	},
	.probe = led_probe,
	.remove = led_remove,
};

/* macro defined by linux kernel, to define register and unregister function */
module_platform_driver(led_drv);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("jason416 <jason416@foxmail,com>");
MODULE_DESCRIPTION("A simple character device driver for LED on itop4412 board");
