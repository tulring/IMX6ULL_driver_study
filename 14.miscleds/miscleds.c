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
#include <linux/miscdevice.h>


#define MISCLED_NAME    "miscled"
#define MISCLED_MINOR   144

#define LEDOFF      0   //关灯
#define LEDON       1   //开灯


//gpioled设备结构体
struct  miscled_dev
{
    /* data */
    dev_t devid;                //设备号
    struct cdev cdev;           //cdev
    struct class *class;        //类
    struct device *device;      //设备
    struct device_node *node;   //LED设备节点
    int led0;                   //LED灯GPIO编号    
};

struct miscled_dev miscled;//led设备
/*
@description        :   led打开或关闭
@param - sta        :   LEDON    
@return             :   无
*/
void led_switch(u8 sta)
{
    u32 val = 0;
    if(sta == LEDON){
        gpio_set_value(miscled.led0,0);
    }
    else if (sta == LEDOFF)
    {
        gpio_set_value(miscled.led0,1);
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
    filp->private_data = &miscled;        //设置私有数据
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
    struct miscled_dev *dev = filp->private_data;

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
/*  MISC设备结构体  */
static struct miscdevice led_miscdev = {
    .minor = MISCLED_MINOR,
    .name = MISCLED_NAME,
    .fops = &led_fops,
};
/*
@description      :   platform 驱动的 probe函数，驱动与设备匹配之后会执行此函数。
@param - dev      :   platform 设备   
@return           :   0，成功，其他负值，失败
*/
static int led_probe(struct platform_device *dev)
{

    int ret = 0;
    printk("led driver and device has matched!\r\n");


    /*   设置led的GPIO   */
    //获取节点
    miscled.node = of_find_node_by_path("/light");
    if(miscled.node == NULL)
    {
        printk("light node not find!\r\n");
        return -EINVAL;
    }

    //获取GPIO属性，得到IO编号
    miscled.led0 = of_get_named_gpio(miscled.node,"light-gpio",0);
    if(miscled.led0 < 0)
    {
        printk("light-gpio not find!\r\n");
        return -EINVAL;
    }
    
    //设置gpio为输出，并且为高电平，默认关闭led
    ret = gpio_direction_output(miscled.led0,1);
    if (miscled.led0 < 0)
    {
        printk("can't set gpio!\r\n");
    }


    /*  注册MISC驱动    */
    ret = misc_register(&led_miscdev);
    if(ret < 0)
    {
        printk("misc device register failed!\r\n");
        return -ENAVAIL;
    }
    
    return 0;

}

/*
@description      :   移除platform 驱动时会执行此函数。
@param - dev      :   platform 设备   
@return           :   0，成功，其他负值，失败
*/
static int led_remove(struct platform_device *dev)
{

    gpio_set_value(miscled.led0,1);//恢复默认值

    misc_deregister(&led_miscdev);

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