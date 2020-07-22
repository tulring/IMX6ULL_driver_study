#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/*  寄存器物理地址  */
#define CCM_CCGR1_BASE              (0x020C406C)
#define SW_MUX_GPIO1_IO04_BASE      (0x020E006C)
#define SW_PAD_GPIO1_IO04_BASE      (0x020E02F8)
#define GPIO1_DR_BASE               (0x0209C000)
#define GPIO1_GDIR_BASE             (0x0209C004)
#define REGISTER_LENGTH             4
/*
* @description      : 释放 platform 设备模块的时候会执行
* @param - dev      : 要释放的设备
* @return           : 无
*/
static void led_release(struct device *dev)
{
    printk("led device released!\r\n");
}

/*
 *  设备资源信息，也就是led使用到的资源信息
 */
static struct resource led_resource[] = {
    [0] = {
        .start = CCM_CCGR1_BASE,
        .end = (CCM_CCGR1_BASE + REGISTER_LENGTH - 1),
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = SW_MUX_GPIO1_IO04_BASE,
        .end = (SW_MUX_GPIO1_IO04_BASE + REGISTER_LENGTH - 1),
        .flags = IORESOURCE_MEM,
    },
    [2] = {
        .start = SW_PAD_GPIO1_IO04_BASE,
        .end = (SW_PAD_GPIO1_IO04_BASE + REGISTER_LENGTH - 1),
        .flags = IORESOURCE_MEM,
    },
    [3] = {
        .start = GPIO1_DR_BASE,
        .end = (GPIO1_DR_BASE + REGISTER_LENGTH - 1),
        .flags = IORESOURCE_MEM,
    },
    [4] = {
        .start = GPIO1_GDIR_BASE,
        .end = (GPIO1_GDIR_BASE + REGISTER_LENGTH - 1),
        .flags = IORESOURCE_MEM,
    },
};

/*
 *  platform 设备结构体
 */
static struct platform_device leddevice = {
    .name = "imx6ul-led",
    .id = -1,
    .dev =  {
        .release = &led_release,
    },
    .num_resources = ARRAY_SIZE(led_resource),
    .resource = led_resource,
};

/*
* @description      : 设备模块加载
* @param - dev      : 无
* @return           : 无
*/
static int __init leddevice_init(void)
{
    return platform_device_register(&leddevice);
}

/*
* @description      : 设备模块注销
* @param - dev      : 无
* @return           : 无
*/
static void __exit leddevice_exit(void)
{
    return platform_device_register(&leddevice);
}


module_init(leddevice_init);
module_exit(leddevice_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Turing");