/*************************************************************************
 * *    > File Name: adc_drv.c
 * *    > Author: jason416
 * *    > Mail: jason416@foxmail.com 
 * *    > Created Time: Sat 22 Aug 2019 10:06:21 PM CST
 * *    > License: GPL v2
 * *************************************************************************/

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

#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/clk.h>

#include "adc.h"

#define DEBUG
#ifdef DEBUG
#define DEBUG_INFO(fmt, arg...) printk(fmt, ##arg)
#else
#define DEBUG_INFO(fmt, arg...)
#endif

#define M               (1000 * 1000) /* for Hz */
#define ADC_MAJOR       256 
#define ADC_MINOR       5 
#define ADC_DEV_NAME    "adc"


/* custom struct to save */
struct adc_dev {
    unsigned int __iomem *adc_con;
    unsigned int __iomem *adc_dat;
    unsigned int __iomem *clr_int;
    unsigned int __iomem *adc_mux;

    unsigned int adc_val;           /* save value */
    struct completion completion;   /* completion */
    atomic_t available;             /* available */
    unsigned int irq;               /* irq */
    struct cdev cdev;               /* cdev */
    struct device *device;          /* device */
    struct clk *clk;                /* clock */ 
    unsigned long freq;             /* frequency */
};

/* for mdev auto-create class node in /sys */
static struct class *adc_cls;

static int adc_open(struct inode *inode, struct file *filp)
{
    struct adc_dev *adc_dev = container_of(inode->i_cdev, struct adc_dev, cdev);

    filp->private_data = adc_dev;
    if(atomic_dec_and_test(&adc_dev->available))
        return 0;
    else {
        atomic_inc(&adc_dev->available);
        return -EBUSY;
    }
}

static int adc_release(struct inode *inode, struct file *filp)
{
    struct adc_dev *adc_dev = filp->private_data;

    atomic_inc(&adc_dev->available);
    return 0;
}

static long adc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct adc_dev *adc_dev = filp->private_data;
    union channel_value cv;

    /* check magic */
    if(_IOC_TYPE(cmd) != ADC_MAGIC)
        return -ENOTTY;

    switch(cmd) {
    case ADC_GET_VAL:
        if(copy_from_user(&cv, (union channel_value __user *)arg, sizeof(union channel_value)))
            return -EFAULT;
        if(cv.channel > AIN3)
            return -ENOTTY;
        DEBUG_INFO("get cmd form userspace!\n");

        /* do get AIN value */
        writel(cv.channel, adc_dev->adc_mux);                       /* adc_mux = cv.channle */  
        writel(readl(adc_dev->adc_con) | 0x01, adc_dev->adc_con);   /* adc_con[0] = 1 */

        DEBUG_INFO("start convert...\n");
        /* wait for completion */
        if(wait_for_completion_interruptible(&adc_dev->completion))
            return -ERESTARTSYS;

        /* copy value read from adc to user space */
        cv.value = adc_dev->adc_val & 0xFFF;                        /* 12-bit */
        if(copy_to_user((union channel_value __user *)arg, &cv, sizeof(union channel_value)))
            return -EFAULT;
        DEBUG_INFO("read value: 0x%x\n", cv.value);
        break;
    default:
        return -ENOTTY;
    }

    return 0;
}

/* in isr, we do clear INT, then wake up process waitting for completion */
static irqreturn_t adc_isr(int irq, void *dev_id)
{
    struct adc_dev *adc_dev = (struct adc_dev *)dev_id;

    adc_dev->adc_val = readl(adc_dev->adc_dat); /* read value */
    writel(1, adc_dev->clr_int);                /* clear INT */ 
    complete(&adc_dev->completion);             /* wake up process */

    DEBUG_INFO("in isr_handle, get value: 0x%x\n", adc_dev->adc_val);
    return IRQ_HANDLED;
}

static struct file_operations adc_ops = {
    .owner = THIS_MODULE,
    .open = adc_open,
    .release = adc_release,
    .unlocked_ioctl = adc_ioctl,
};

/*
 * Do init register and allocate resource
 */
static int adc_probe(struct platform_device *pdev)
{
    int ret;
    dev_t dev;
    struct resource *res;
    struct adc_dev *adc_dev;
    unsigned long prescal_value; 

    DEBUG_INFO("in probe function!\n");
    dev = MKDEV(ADC_MAJOR, ADC_MINOR);
    ret = register_chrdev_region(dev, 1, ADC_DEV_NAME);
    if(ret)
        goto reg_err;

    adc_dev = kzalloc(sizeof(struct adc_dev), GFP_KERNEL);
    if(!adc_dev) {
        ret = -ENOMEM;
        goto mem_err;
    }

    cdev_init(&adc_dev->cdev, &adc_ops);
    adc_dev->cdev.owner = THIS_MODULE;
    ret = cdev_add(&adc_dev->cdev, dev, 1);
    if(ret)
        goto add_err;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(!res) {
        return -ENOENT;
        goto res_err;
    }

    adc_dev->adc_con = ioremap(res->start, resource_size(res));
    if(!adc_dev->adc_con) {
        ret = -EBUSY;
        goto map_err;
    }
    adc_dev->adc_dat = adc_dev->adc_con + 3;    /* Note: int * + 3, offset is 24 bytes */
    adc_dev->clr_int = adc_dev->adc_con + 6;
    adc_dev->adc_mux = adc_dev->adc_con + 7;

    adc_dev->irq = platform_get_irq(pdev, 0);
    if(adc_dev->irq < 0) {
        ret = adc_dev->irq;
        goto irq_err;
    }

    /* adc is a SHARED_IRQ, MUST give args(to isr handler) */
    ret = request_irq(adc_dev->irq, adc_isr, 0, "adc", adc_dev);
    if(ret)
        goto irq_err;

#if 1
    DEBUG_INFO("ready to create device in /sys/class(as root).\n");
    /* create device, to match class(/sys/class/), work with mdev */
    adc_dev->device = device_create(adc_cls, NULL, dev, NULL, ADC_DEV_NAME);
    if(IS_ERR(adc_dev->device)) {
        DEBUG_INFO("device_create error!\n");
        ret = PTR_ERR(adc_dev->device);
        goto dev_err;
    }
#endif
    /* praper data for matched device */
    platform_set_drvdata(pdev, adc_dev);

    /*****************************get clock**************************/
    adc_dev->clk = clk_get(&pdev->dev, "adc");
    if(IS_ERR(adc_dev->clk)) {
        DEBUG_INFO("get clock failed!\n");
        ret = PTR_ERR(adc_dev->clk);
        goto get_clk_err;
    }

    ret = clk_prepare_enable(adc_dev->clk);
    if(ret < 0)
        goto enable_clk_err;

    DEBUG_INFO("enable clock success! ready to get frequency.\n");
    adc_dev->freq = clk_get_rate(adc_dev->clk);
    DEBUG_INFO("clock freq = %lu\n", adc_dev->freq);
    /***************************************************************/
    /*
     * ADCCON[16] = 1    ----> 12bit
     * ADCCON[14] = 1    ----> enable perscale
     * ADCCON[13:6] = 19 ----> perscale value = (freq+5M-1)/(5M) - 1 [round up]
     *     here is: (adc_dev->freq + 5M-1) / 5M - 1 --> ADCCON[13:6]
     *     on my system freq is 133,333,333Hz, so ADCCON[13:6] = 26!
     */
    prescal_value = (adc_dev->freq + 5 * M - 1) / (5 * M) - 1;
    DEBUG_INFO("using resolution: 12-bits, prescal_value: %lu.\n", prescal_value);
    writel((1 << 16) | (1 << 14) | (prescal_value << 6), adc_dev->adc_con);
    /***************************************************************/

    /* init completion & set atomit_t var as 1(available) */
    init_completion(&adc_dev->completion);
    atomic_set(&adc_dev->available, 1);

    DEBUG_INFO("device probe normally, exit 0.\n");
    return 0;

enable_clk_err:
    free_irq(adc_dev->irq, adc_dev);
get_clk_err:
    clk_put(adc_dev->clk);
dev_err:
    device_destroy(adc_cls, dev);
irq_err:
    iounmap(adc_dev->adc_con);
map_err:
res_err:
    cdev_del(&adc_dev->cdev);
add_err:
    kfree(adc_dev);
mem_err:
    unregister_chrdev_region(dev, 1);
reg_err:
    DEBUG_INFO("device probe failed, exit %d.\n", ret);
    return ret;
}

static int adc_remove(struct platform_device *pdev)
{
    dev_t dev;
    struct adc_dev *adc_dev;

    /* get data from matched device for driver */
    adc_dev = platform_get_drvdata(pdev);
    dev = MKDEV(ADC_MAJOR, ADC_MINOR);

    /*
     * ADCCON[16] = 0 ----> 10bit
     * ADCCON[2]  = 1 ----> standby mode(not start convert)
     */
    writel((readl(adc_dev->adc_con) & ~(1 << 16)) | (1 << 2), adc_dev->adc_con);

    free_irq(adc_dev->irq, adc_dev);
    iounmap(adc_dev->adc_con);
    cdev_del(&adc_dev->cdev);
    kfree(adc_dev);
    device_destroy(adc_cls, dev);
    unregister_chrdev_region(dev, 1);

    return 0;
}

static const struct of_device_id adc_of_matches[] = {
    { .compatible = "samsung,exynos-adc-v1" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, adc_of_matches);

struct platform_driver adc_drv = {
    .driver = {
        .name = "adc",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(adc_of_matches),
    },
    .probe = adc_probe,
    .remove = adc_remove,
};

#if 0
/* macro defined by linux kernel, to define register and unregister function */
module_platform_driver(adc_drv);
#else
static int __init adc_init(void)
{
	int ret;

    DEBUG_INFO("ready to create class in module_init.\n");
	/* create a class, to match dev */
	adc_cls = class_create(THIS_MODULE, ADC_DEV_NAME);
	if(IS_ERR(adc_cls)) {
    DEBUG_INFO("create class error.\n");
		return PTR_ERR(adc_cls);
	}

	ret = platform_driver_register(&adc_drv);
	if(ret)
		class_destroy(adc_cls); /* if register failed */

	return ret;
}

static void __exit adc_exit(void)
{
	platform_driver_unregister(&adc_drv);
	class_destroy(adc_cls);
}

module_init(adc_init);
module_exit(adc_exit);
#endif

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("jason416 <jason416@foxmail,com>");
MODULE_DESCRIPTION("A simple platform device driver for ADC on itop4412 board, use dtb");
