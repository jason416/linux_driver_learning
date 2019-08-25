/*************************************************************************
 * *    > File Name: pwm_drv.c
 * *    > Author: jason416
 * *    > Mail: jason416@foxmail.com 
 * *    > Created Time: Sat 24 Aug 2019 10:06:21 PM CST
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
#include <linux/pinctrl/consumer.h>

#include "pwm.h"

#define DEBUG
#ifdef DEBUG
#define DEBUG_INFO(fmt, arg...) printk(fmt, ##arg)
#else
#define DEBUG_INFO(fmt, arg...)
#endif

#define PWM_MAJOR       256 
#define PWM_MINOR       6

/*
 * kernel has already created a class name pwm, if use AUTO_CREAT_DEV_NODE, this module will crash.
 */
#define PWM_DEV_NAME    "pwm_demo"

#define AUTO_CREAT_DEV_NODE
//define NEED_ISR

/* custom struct to save */
struct pwm_dev {
    unsigned int __iomem *tcfg0;
    unsigned int __iomem *tcfg1;
    unsigned int __iomem *tcon;
    unsigned int __iomem *tcntb0;
    unsigned int __iomem *tcmpb0;
    unsigned int __iomem *tcnto0;
    unsigned int __iomem *tint_cstat;

    struct cdev cdev;               /* cdev */
    atomic_t available;             /* available */
    struct clk *clk;                /* clock */ 
    unsigned long freq;             /* frequency */
    struct pinctrl *pinctrl;        /* pinctrl */
#ifdef NEED_ISR
    unsigned int irq;               /* irq */
#endif
#ifdef AUTO_CREAT_DEV_NODE
    struct device *device;          /* device */
#endif
};

/* for mdev auto-create class node in /sys/class */
#ifdef AUTO_CREAT_DEV_NODE
static struct class *pwm_cls;
#endif

static int pwm_open(struct inode *inode, struct file *filp)
{
    struct pwm_dev *pwm_dev = container_of(inode->i_cdev, struct pwm_dev, cdev);

    filp->private_data = pwm_dev;
    if(atomic_dec_and_test(&pwm_dev->available))
        return 0;
    else {
        atomic_inc(&pwm_dev->available);
        return -EBUSY;
    }
}

static int pwm_release(struct inode *inode, struct file *filp)
{
    struct pwm_dev *pwm_dev = filp->private_data;

    atomic_inc(&pwm_dev->available);
    return 0;
}

static long pwm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct pwm_dev *pwm_dev = filp->private_data;
    struct pwm_arg pwm_arg;

    /* check magic */
    if(_IOC_TYPE(cmd) != PWM_MAGIC)
        return -ENOTTY;

    switch(cmd) {
    case PWM_START:
        DEBUG_INFO("pwm starting...\n");
        writel(readl(pwm_dev->tcon) | 0x01, pwm_dev->tcon);
        break;
    case PWM_STOP:
        DEBUG_INFO("pwm stopping...\n");
        writel(readl(pwm_dev->tcon) & ~0x01, pwm_dev->tcon);
        break;
    case PWM_SET_FREQ:
        if(copy_from_user(&pwm_arg, (struct pwm_arg __user *)arg, sizeof(struct pwm_arg)))
            return -EFAULT;
        if(pwm_arg.freq > pwm_dev->freq || pwm_arg.freq == 0) /* check freq valid */
            return -ENOTTY;

        DEBUG_INFO("set pwm timer's freq: %u.\n", pwm_arg.freq);

        /* 1. Write the initial value into TCNTBn and TCMPBn. */
        writel(pwm_dev->freq / pwm_arg.freq - 1, pwm_dev->tcntb0);     /* set freq, in Hz */
        writel(pwm_dev->freq / pwm_arg.duty_cyle - 1, pwm_dev->tcmpb0); /* duty_cyle = 50% */
        /* 2. Set the manual update bit and clear only manual update bit of the corresponding timer. */
        writel(readl(pwm_dev->tcon) | 0x02, pwm_dev->tcon);
        writel((readl(pwm_dev->tcon) & ~0x02), pwm_dev->tcon);

        if(strncmp(pwm_arg.mode, NORMAL_MODE, sizeof(NORMAL_MODE)) == 0) {
            DEBUG_INFO("settng in %s mode.\n", NORMAL_MODE);
            writel(readl(pwm_dev->tcon) & ~0x4, pwm_dev->tcon);
        }
        if(strncmp(pwm_arg.mode, INVERSED_MODE, sizeof(INVERSED_MODE)) == 0) {
            DEBUG_INFO("settng in %s mode.\n", INVERSED_MODE);
            writel(readl(pwm_dev->tcon) | 0x4, pwm_dev->tcon);
        }

        break;
    case PWM_GET_FREQ:        
        break;
    default:
        return -ENOTTY;
    }

    return 0;
}

/*
 * In this demo, we do not use interrupt of pwm timer.
 */
#ifdef NEED_ISR
/* in isr, we do clear INT, then do something you like */
static irqreturn_t pwm_isr(int irq, void *dev_id)
{
    /* pwm is not a SPI, dev_id can be NULL */

    /* DO SOMETHING quickly */
    writel(readl(pwm_dev->tint_cstat) | (1 << 5), pwm_dev->tint_cstat); /* clear INT */ 

    DEBUG_INFO("in isr_handle, irq = %d\n", irq);
    return IRQ_HANDLED;
}
#endif

static struct file_operations pwm_ops = {
    .owner = THIS_MODULE,
    .open = pwm_open,
    .release = pwm_release,
    .unlocked_ioctl = pwm_ioctl,
};

/*
 * Do init register and allocate resource
 */
static int pwm_probe(struct platform_device *pdev)
{
    int ret;
    dev_t dev;
    struct resource *res;
    struct pwm_dev *pwm_dev;
    unsigned long prescaler0; 

    DEBUG_INFO("in probe function!\n");
    dev = MKDEV(PWM_MAJOR, PWM_MINOR);
    ret = register_chrdev_region(dev, 1, PWM_DEV_NAME);
    if(ret)
        goto reg_err;

    pwm_dev = kzalloc(sizeof(struct pwm_dev), GFP_KERNEL);
    if(!pwm_dev) {
        ret = -ENOMEM;
        goto mem_err;
    }

    cdev_init(&pwm_dev->cdev, &pwm_ops);
    pwm_dev->cdev.owner = THIS_MODULE;
    ret = cdev_add(&pwm_dev->cdev, dev, 1);
    if(ret)
        goto add_err;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(!res) {
        return -ENOENT;
        goto res_err;
    }

    pwm_dev->tcfg0 = ioremap(res->start, resource_size(res));
    if(!pwm_dev->tcfg0) {
        ret = -EBUSY;
        goto map_err;
    }
    pwm_dev->tcfg1  = pwm_dev->tcfg0 + 1;    /* Note: int * + 3, offset is 24 bytes */
    pwm_dev->tcon   = pwm_dev->tcfg0 + 2;
    pwm_dev->tcntb0 = pwm_dev->tcfg0 + 3;
    pwm_dev->tcmpb0 = pwm_dev->tcfg0 + 4;
    pwm_dev->tcnto0 = pwm_dev->tcfg0 + 5;

#ifdef NEED_ISR
    pwm_dev->tint_cstat = pwm_dev->tcg0 + 17;
    pwm_dev->irq = platform_get_irq(pdev, 0);
    if(pwm_dev->irq < 0) {
        ret = pwm_dev->irq;
        goto irq_err;
    }

    /* pwm is not a SHARED_IRQ,  args(to isr handler) can be NULL */
    ret = request_irq(pwm_dev->irq, pwm_isr, 0, "pwm", NULL);
    if(ret)
        goto irq_err;
#endif

#if defined(AUTO_CREAT_DEV_NODE)
    DEBUG_INFO("ready to create device in /sys/class(as root).\n");
    /* create device, to match class(/sys/class/), work with mdev */
    pwm_dev->device = device_create(pwm_cls, NULL, dev, NULL, PWM_DEV_NAME);
    if(IS_ERR(pwm_dev->device)) {
        DEBUG_INFO("device_create error!\n");
        ret = PTR_ERR(pwm_dev->device);
        goto dev_err;
    }
#endif
    /* praper data for matched device */
    platform_set_drvdata(pdev, pwm_dev);

    /*****************************get clock**************************/
    pwm_dev->clk = clk_get(&pdev->dev, "timers");
    if(IS_ERR(pwm_dev->clk)) {
        DEBUG_INFO("get clock failed!\n");
        ret = PTR_ERR(pwm_dev->clk);
        goto get_clk_err;
    }

    ret = clk_prepare_enable(pwm_dev->clk);
    if(ret < 0)
        goto enable_clk_err;

    DEBUG_INFO("enable clock success! ready to get frequency.\n");
    pwm_dev->freq = clk_get_rate(pwm_dev->clk);
    DEBUG_INFO("clock freq = %lu\n", pwm_dev->freq);
    /***************************************************************/
    /*
     * TCFG0[7:0]      ---> Timer0 prescaler
     * TCFG1[3:0] = 4  ---> DMUX0 = 0100; means 1/16
     * TCON[3:0]  = 8  ---> Timer0: auto-reload, inverter on, No manual update, stop.
     */
    prescaler0 = readl(pwm_dev->tcfg0) & 0xff;                      /* timer0 prescaler */
    DEBUG_INFO("prescal0: %lu.\n", prescaler0);
    writel((readl(pwm_dev->tcfg1) & ~0x0f) | 0x04, pwm_dev->tcfg1); /* 1/16 */
    pwm_dev->freq /= (prescaler0 + 1) * 16;                         /* max frequency */

    /*
     * on itop4412, only inverter ON make output LOW when timer stopped.
     * because bepp is active bepper!
     */
    writel((readl(pwm_dev->tcon) & ~0x0f) | 0x0c, pwm_dev->tcon);

    /*
     * Set period as 0, duty_cyle as 0 too.
     */
    writel(0, pwm_dev->tcntb0);     /* set freq, in Hz */
    writel(0, pwm_dev->tcmpb0);     /* duty_cyle */
    writel(readl(pwm_dev->tcon) | 0x2, pwm_dev->tcon);
    writel((readl(pwm_dev->tcon) & ~0x2), pwm_dev->tcon);
    /***************************************************************/
    /* get default pinctrl, and DO SETUP(done in kernel, we do not care) */
    pwm_dev->pinctrl = devm_pinctrl_get_select_default(&pdev->dev);

    atomic_set(&pwm_dev->available, 1);

    DEBUG_INFO("device probe normally, exit 0.\n");
    return 0;

enable_clk_err:
    clk_put(pwm_dev->clk);
get_clk_err:
#ifdef NEED_ISR
    free_irq(pwm_dev->irq, pwm_dev);
#endif
#if defined(AUTO_CREAT_DEV_NODE)
dev_err:
    device_destroy(pwm_cls, dev);
#endif
#ifdef NEED_ISR
irq_err:
#endif
    iounmap(pwm_dev->tcfg0);
map_err:
res_err:
    cdev_del(&pwm_dev->cdev);
add_err:
    kfree(pwm_dev);
mem_err:
    unregister_chrdev_region(dev, 1);
reg_err:
    DEBUG_INFO("device probe failed, exit %d.\n", ret);
    return ret;
}

static int pwm_remove(struct platform_device *pdev)
{
    dev_t dev;
    struct pwm_dev *pwm_dev;

    /* get data from matched device for driver */
    pwm_dev = platform_get_drvdata(pdev);
    dev = MKDEV(PWM_MAJOR, PWM_MINOR);

    /* stop timer */
    writel(readl(pwm_dev->tcon) & ~0x01, pwm_dev->tcon);

#ifdef NEED_ISR
    free_irq(pwm_dev->irq, pwm_dev);
#endif
    clk_disable_unprepare(pwm_dev->clk);
    clk_put(pwm_dev->clk);
    iounmap(pwm_dev->tcfg0);
    cdev_del(&pwm_dev->cdev);
    kfree(pwm_dev);
#if defined(AUTO_CREAT_DEV_NODE)
    device_destroy(pwm_cls, dev);
#endif
    unregister_chrdev_region(dev, 1);

    return 0;
}

static const struct of_device_id pwm_of_matches[] = {
    { .compatible = "samsung,exynos4210-pwm" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, pwm_of_matches);

struct platform_driver pwm_drv = {
    .driver = {
        .name = "pwm",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(pwm_of_matches),
    },
    .probe = pwm_probe,
    .remove = pwm_remove,
};

#if !defined(AUTO_CREAT_DEV_NODE)
/* macro defined by linux kernel, to define register and unregister function */
module_platform_driver(pwm_drv);
#else
static int __init pwm_init(void)
{
	int ret;

    DEBUG_INFO("ready to create class in module_init.\n");
	/* create a class, to match dev */
	pwm_cls = class_create(THIS_MODULE, PWM_DEV_NAME);
	if(IS_ERR(pwm_cls)) {
    DEBUG_INFO("create class error.\n");
		return PTR_ERR(pwm_cls);
	}

	ret = platform_driver_register(&pwm_drv);
	if(ret)
		class_destroy(pwm_cls); /* if register failed */

	return ret;
}

static void __exit pwm_exit(void)
{
	platform_driver_unregister(&pwm_drv);
	class_destroy(pwm_cls);
}

module_init(pwm_init);
module_exit(pwm_exit);
#endif

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("jason416 <jason416@foxmail,com>");
MODULE_DESCRIPTION("A simple platform device driver for pwm on itop4412 board, use dtb");
