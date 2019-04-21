/*************************************************************************
*    > File Name: led_dev.c
*    > Author: jason416
*    > Mail: jason416@foxmail.com 
*    > Created Time: Sat 20 Apr 2019 11:07:55 PM CST
*    > License: GPL v2
*************************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/platform_device.h>

#define GPL2CON_BASE 0x11000100
#define GPK1CON_BASE 0x11000060

static void led_release(struct device *dev)
{
	/* do nothing right now */
}

/* GPL2_0 and GPK1_1 */
static struct resource led1_res[] = {
	[0] = DEFINE_RES_MEM(GPL2CON_BASE, 4 + 4),
};
static struct resource led2_res[] = {
	[0] = DEFINE_RES_MEM(GPK1CON_BASE, 4 + 4),
};

unsigned int led1_pin= 0;
unsigned int led2_pin= 1;

/* platform_device struct for led1 */
struct platform_device led1 = {
	.name = "led",
	.id = 1,
	.num_resources = ARRAY_SIZE(led1_res),
	.resource = led1_res,
	.dev = {
		.release = led_release,
		.platform_data = &led1_pin, /* led1's private data, pass to driver */
	},
};
/* platform_device struct for led2 */
struct platform_device led2 = {
	.name = "led",
	.id = 2,
	.num_resources = ARRAY_SIZE(led2_res),
	.resource = led2_res,
	.dev = {
		.release = led_release,
		.platform_data = &led2_pin, /* led2's private data, pin number here */
	},
};

/* platform_device array */
static struct platform_device *led_devices[] = {
	&led1, &led2,
};

/* add sturct, which decribe a physic led device */
static int __init led_init(void)
{
	return platform_add_devices(led_devices, ARRAY_SIZE(led_devices));
}

static void __exit led_exit(void)
{
	platform_device_unregister(&led1);
	platform_device_unregister(&led2);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("jason416 <jason416@foxail.com>");
MODULE_DESCRIPTION("register LED device");
