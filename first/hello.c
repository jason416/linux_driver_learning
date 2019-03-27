/*************************************************************************
*    > File Name: ver.c
*    > Author: jason416
*    > Mail: jason416@foxmail.com 
*    > Created Time: Tue 26 Mar 2019 08:20:22 AM PDT
*************************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

static __init int ver_init(void)
{
	printk("first module, init!\n");
	return 0;
}

static __exit void ver_exit(void)
{
	printk("first module, exit!\n");
}

module_init(ver_init);
module_exit(ver_exit);

MODULE_LICENSE("GPL v2");
