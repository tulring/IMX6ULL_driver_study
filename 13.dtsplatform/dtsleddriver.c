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
#include <linux/of_gpio.h>
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

#define LEDDEV_CNT           1
#define LEDDEV_NAME          "dtsplatled"

#define LEDOFF      0   //关灯
#define LEDON       1   //开灯


//gpioled设备结构体
struct  leddev_dev
{
    /* data */
    dev_t devid;                //设备号
    struct cdev cdev;           //cdev
    struct class *class;        //类
    struct device *device;      //设备
    int major;                  //主设备号
    struct device_node *node;   //LED设备节点
    int led0;                   //LED灯GPIO编号    
};

struct leddev_dev leddev;//led设备
/*
@description        :   led打开或关闭
@param - sta        :   LEDON    
@return             :   无
*/
void led_switch(u8 sta)
{
    u32 val = 0;
    if(sta == LEDON){
        gpio_set_value(leddev.led0,0);
    }
    else if (sta == LEDOFF)
    {
        gpio_set_value(leddev.led0,1);
    }
}




/*
@description      :   打开设备
@param - inode    :   传递给驱动的inode   
@param - filp     :   设备文件 
@return           :   0 成功； 其他 失败
*/
static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &leddev;        //设置私有数据
    return 0;
}

/*
@description      :   向设备写数据
@param - filp     :   设备文件，表示打开的文件描述符   
@param - buf      :   写入的数据
@param - cnt      :   数据长度 
@param - offt     :   相对于文件首地址的偏移
@return           :   写入的字节数，如果<0 写入失败
*/
static ssize_t led_write(struct file *filp,const char __user *buf,size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[2];
    unsigned char ledstat;
    retvalue = copy_from_user(databuf,buf,cnt);
    if(retvalue < 0)
        return -EFAULT;
    
    ledstat = databuf[0];
    if(ledstat == LEDON)
    {
        led_switch(LEDON);
    }
    else if (ledstat == LEDOFF)
    {
        led_switch(LEDOFF);
    }

    return 0;
}

/*   设备操作函数集合   */
static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
};
/*
@description      :   platform 驱动的 probe函数，驱动与设备匹配之后会执行此函数。
@param - dev      :   platform 设备   
@return           :   0，成功，其他负值，失败
*/
static int led_probe(struct platform_device *dev)
{
    // int i = 0;
    // int ressize[5];
    // u32 val = 0;
    // struct resource *ledsource[5];

    printk("led driver and device has matched!\r\n");


    //注册字符设备驱动

    //1.创建设备号
    if(leddev.major)
    {
        leddev.devid = MKDEV(leddev.major,0);
        register_chrdev_region(leddev.devid,LEDDEV_CNT,LEDDEV_NAME);
    }
    else
    {
        alloc_chrdev_region(&leddev.devid,0,LEDDEV_CNT,LEDDEV_NAME);//申请设备号
        leddev.major = MAJOR(leddev.devid);//获取主设备号

    }

    printk("leddev major=%d\r\n",leddev.major);
    
    //2.初始化cdev
    leddev.cdev.owner = THIS_MODULE;
    cdev_init(&leddev.cdev,&led_fops);

    //3.添加cdev
    cdev_add(&leddev.cdev,leddev.devid,LEDDEV_CNT);

    //4.创建类
    leddev.class = class_create(THIS_MODULE,LEDDEV_NAME);
    if(IS_ERR(leddev.class))
    {
        return PTR_ERR(leddev.class);
    }
    //5.创建设备
    leddev.device = device_create(leddev.class,NULL,leddev.devid,NULL,LEDDEV_NAME);
    if(IS_ERR(leddev.device))
    {
        return PTR_ERR(leddev.device);
    }

    /*  6.初始化IO  */
    leddev.node = of_find_node_by_path("/light");
    if(leddev.node == NULL)
    {
        printk("gpioled node not find!\r\n");
        return -EINVAL;
    }

    leddev.led0 = of_get_named_gpio(leddev.node,"light-gpio",0);
    if(leddev.led0 < 0)
    {
        printk("can not get light-gpio!\r\n");
        return -EINVAL;
    }

    gpio_request(leddev.led0,"led0");
    gpio_direction_output(leddev.led0,1);

    return 0;
}

/*
@description      :   移除platform 驱动时会执行此函数。
@param - dev      :   platform 设备   
@return           :   0，成功，其他负值，失败
*/
static int led_remove(struct platform_device *dev)
{

    gpio_set_value(leddev.led0,1);//恢复默认值

    cdev_del(&leddev.cdev); //删除cdev
    unregister_chrdev_region(leddev.devid,LEDDEV_CNT);
    device_destroy(leddev.class,leddev.devid);
    class_destroy(leddev.class);

    return 0;

}

/*
@description      :   platform 匹配列表。
*/
static const struct of_device_id led_of_match[] = 
{
    {   .compatible = "gpio-light"  },
    {       }
};

/*
@description      :   platform 驱动结构体。
*/
static struct platform_driver led_driver = {
    .driver = {
        .name = "gpio-light",   //驱动名字，用于和设备匹配
        .of_match_table = led_of_match,
    },
    .probe = led_probe,
    .remove = led_remove,
};
/*
@description      :   驱动模块加载函数。
@param            :   无   
@return           :   无
*/
static void __init leddriver_init(void)
{
    return platform_driver_register(&led_driver);
}

/*
@description      :   驱动模块卸载函数。
@param            :   无   
@return           :   无
*/
static void __exit leddriver_exit(void)
{
    platform_driver_unregister(&led_driver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Turing");