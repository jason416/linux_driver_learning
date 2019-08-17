/*************************************************************************
*    > File Name: key_drv.c
*    > Author: jason416
*    > Mail: jason416@foxmail.com 
*    > Created Time: Tue 06 Aug 2019 12:33:56 AM CST
*    > License: GPL v2
*************************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

struct resource *key_home_res;
struct resource *key_back_res;

/* interrupt handler */
static irqreturn_t key_handler(int irq, void *dev_id)
{
	if(irq == key_home_res->start)
		printk("Home key, the GPX1_1, pressed!\n");
	else
		printk("Back key, the GPX1_2, pressed!\n");

	return IRQ_HANDLED;
}

static int key_probe(struct platform_device *pdev)
{
	int ret;

	/* get platform resource, type IORESOURCE_IRQ */
	key_home_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	key_back_res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);

	if(!(key_home_res && key_back_res)) {
		ret = -ENOENT;
		goto res_err;
	}

	/* register irq in kernel */
	ret = request_irq(key_home_res->start, key_handler, key_home_res->flags & IRQF_TRIGGER_MASK, "key_home", NULL);
	if(ret)
		goto key_home_err;
	ret = request_irq(key_back_res->start, key_handler, key_back_res->flags & IRQF_TRIGGER_MASK, "key_back", NULL);
	if(ret)
		goto key_back_err;

	return 0;

key_back_err:
	free_irq(key_home_res->start, NULL);
key_home_err:
res_err:
	return ret;
}

static int key_remove(struct platform_device *pdev)
{
	/* unregister irq */
	free_irq(key_back_res->start, NULL);
	free_irq(key_home_res->start, NULL);

	return 0;
}

/* match node in devicetree */
static const struct of_device_id key_of_matches[] = {
	/* compatible mst be exactly the same as the node in devicetree, EVEN SPACE!!! */
	{ .compatible = "itop4412, key", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, key_of_matches);

struct platform_driver key_drv = {
	.driver = {
		/* must exactly the same as the 'compatible' property of node in devicetree */
		.name = "key",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(key_of_matches),
	},
	.probe = key_probe,
	.remove = key_remove,
};

/* macro for module_int and module_exit */
module_platform_driver(key_drv);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("jason416 <jason416@foxmail,com>");
MODULE_DESCRIPTION("A simple character device driver for KEY on itop4412 board");

