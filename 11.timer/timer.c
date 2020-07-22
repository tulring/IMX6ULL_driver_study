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
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define TIMER_CNT           1
#define TIMER_NAME          "timer"
#define CLOSE_CMD           (_IO(0xEF,0x1))     //关闭定时器
#define OPEN_CMD            (_IO(0xEF,0x2))     //打开定时器
#define SETPERIOD_CMD       (_IO(0xEF,0x3))     //设置定时器周期命令

#define LEDOFF      0   //关灯
#define LEDON       1   //开灯


//gpioled设备结构体
struct  timer_dev
{
    /* data */
    dev_t devid;                //设备号
    struct cdev cdev;           //cdev
    struct class *class;        //类
    struct device *device;      //设备
    int major;                  //主设备号
    int minor;                  //次设备号
    struct  device_node *nd;    //设备节点
    int led_gpio;               //使用的GPIO编号
    int timeperiod;             //定时器周期，单位ms
    struct timer_list timer;    //定义定时器
    spinlock_t lock;            //定义自旋锁
};

struct timer_dev timerdev;//timer设备
/*
初始化led IO，open函数打开驱动时初始化GPIO
*/
static int led_init(void)
{
    
    timerdev.nd = of_find_node_by_path("/light");//获取设备节点
    if(timerdev.nd == NULL)
    {
        return -EINVAL;
    }
    timerdev.led_gpio = of_get_named_gpio(timerdev.nd,"light-gpio",0);//获取GPIO编号
    if(timerdev.led_gpio < 0)
    {
        printk("can't find light-gpio!\r\n");
        
    }
    return 0;
}

/*
@description      :   打开设备
@param - inode    :   传递给驱动的inode   
@param - filp     :   设备文件 
@return           :   0 成功； 其他 失败
*/
static int timer_open(struct inode *inode, struct file *file)
{
    int ret = 0;
    file->private_data = &timerdev;        //设置私有数据
    
    timerdev.timeperiod = 1000;//默认定时器为1s
    ret = led_init();
    if(ret < 0)
    {
        return ret;
    }
    return 0;
}
/*
ioctl函数

*/
static long timer_unlocked_ioctl(struct file *filp,unsigned int cmd, unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev *)filp->private_data;
    int timerperiod;
    unsigned long flags;

    switch (cmd)
    {
    case CLOSE_CMD:
        del_timer_sync(&dev->timer);
        break;
    case OPEN_CMD:
        spin_lock_irqsave(&dev->lock,flags);
        timerperiod = dev->timeperiod;
        spin_unlock_irqrestore(&dev->lock,flags);
        mod_timer(&dev->timer,jiffies + msecs_to_jiffies(timerperiod));
        break;
    case SETPERIOD_CMD:
        spin_lock_irqsave(&dev->lock,flags);
        dev->timeperiod = arg;

        spin_unlock_irqrestore(&dev->lock,flags);
        mod_timer(&dev->timer,jiffies + msecs_to_jiffies(arg));
        break;
    default:
        break;
    }
    return 0;
}






/*   设备操作函数集合   */
static struct file_operations timer_fops = {
    .owner = THIS_MODULE,
    .open = timer_open,
    .unlocked_ioctl = timer_unlocked_ioctl,
};

void timer_function(unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev*)arg;
    static int sta = 1;
    int timerperiod;
    unsigned long flags;
    sta = !sta;//每次状态取反，翻转LED

    gpio_set_value(dev->led_gpio,sta);

    //重启定时器
    spin_lock_irqsave(&dev->lock,flags);
    timerperiod = dev->timeperiod;
    spin_unlock_irqrestore(&dev->lock,flags);
    mod_timer(&dev->timer,jiffies + msecs_to_jiffies(dev->timeperiod));

}

/*
@description      :   驱动入口函数  
@param -          :   无 
@return           :   无
*/
static int __init timer_init(void)
{
    //初始化自旋锁
    spin_lock_init(&timerdev.lock);


    
    //注册字符设备驱动

    //1.创建设备号
    if(timerdev.major)
    {
        timerdev.devid = MKDEV(timerdev.major,0);
        register_chrdev_region(timerdev.devid,TIMER_CNT,TIMER_NAME);
    }
    else
    {
        alloc_chrdev_region(&timerdev.devid,0,TIMER_CNT,TIMER_NAME);//申请设备号
        timerdev.major = MAJOR(timerdev.devid);//获取主设备号
        timerdev.minor = MINOR(timerdev.devid);//获取次设备号
    }

    printk("timerdev major=%d,minor=%d\r\n",timerdev.major,timerdev.minor);

    //2.初始化cdev
    timerdev.cdev.owner = THIS_MODULE;
    cdev_init(&timerdev.cdev,&timer_fops);

    //3.添加cdev
    cdev_add(&timerdev.cdev,timerdev.devid,TIMER_CNT);

    //4.创建类
    timerdev.class = class_create(THIS_MODULE,TIMER_NAME);
    if(IS_ERR(timerdev.class))
    {
        return PTR_ERR(timerdev.class);
    }
    //5.创建设备
    timerdev.device = device_create(timerdev.class,NULL,timerdev.devid,NULL,TIMER_NAME);
    if(IS_ERR(timerdev.device))
    {
        return PTR_ERR(timerdev.device);
    }

    //6.初始化timer，设置定时器回调函数，未设置周期，因此不会激活定时器
    init_timer(&timerdev.timer);
    timerdev.timer.function = timer_function;
    timerdev.timer.data = (unsigned long)&timerdev;
    return 0;

}

static void __exit timer_exit(void)
{


    gpio_set_value(timerdev.led_gpio,1);
    del_timer_sync(&timerdev.timer);

    //注销字符驱动
    cdev_del(&timerdev.cdev);//删除cdev
    unregister_chrdev_region(timerdev.devid,TIMER_CNT);
    device_destroy(timerdev.class,timerdev.devid);
    class_destroy(timerdev.class);
    
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Turing");