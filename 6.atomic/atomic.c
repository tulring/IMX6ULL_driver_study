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

#define GPIOLED_CNT      1  //设备号个数
#define GPIOLED_NAME         "gpioled"

#define LEDOFF      0   //关灯
#define LEDON       1   //开灯


//gpioled设备结构体
struct  gpioled_dev
{
    /* data */
    dev_t devid;                //设备号
    struct cdev cdev;           //cdev
    struct class *class;        //类
    struct device *device;      //设备
    int major;                  //主设备号
    int minor;                  //次设备号
    struct  device_node *nd;    //设备节点
    int led_gpio;               //led使用的GPIO编号
    atomic_t lock;              //原子变量
};

struct gpioled_dev gpioled;//led设备



/*
@description      :   打开设备
@param - inode    :   传递给驱动的inode   
@param - filp     :   设备文件 
@return           :   0 成功； 其他 失败
*/
static int led_open(struct inode *inode, struct file *file)
{
    /*  通过原子变量的值检查LED有没有被别的应用使用 */
    if(!atomic_dec_and_test(&gpioled.lock))
    {
        atomic_inc(&gpioled.lock);//小于0就加一，使其变0
        return -EBUSY;//LED被使用，返回忙
    }

    file->private_data = &gpioled;        //设置私有数据
    return 0;
}


/*
@description      :   从设备读取数据 
@param - filp     :   要打开的设备文件
@param - buf      :   返回给用户控件的数据缓冲区
@param - cnt      :   要读取的数据长度
@param - offt     :   相对于文件首地址的偏移      
@return           :   读取的字节数，若为负值，读取失败
*/
static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt,loff_t *offt)
{
    return 0;
}

/*
@description      :   向设备写数据 
@param - filp     :   设备文件
@param - buf      :   要写入设备的数据
@param - cnt      :   要写入的数据长度
@param - offt     :   相对于文件首地址的偏移      
@return           :   写入的字节数，若为负值，读取失败
*/
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt,loff_t *offt)
{
    int retvalue;
    unsigned char datebuf[1];
    unsigned char ledstat;
    struct gpioled_dev *dev = filp->private_data;

    
    retvalue = copy_from_user(datebuf, buf, cnt);
    if(retvalue < 0)
    {
        printk("kernel write failed!!\r\n");
        return -EFAULT;
    }

    ledstat = datebuf[0];

    if(ledstat  == LEDON)
    {
        gpio_set_value(dev->led_gpio,0);//打开led灯
    }    
    else if (ledstat == LEDOFF)
    {
        gpio_set_value(dev->led_gpio,1);//关闭led灯
    }
    return 0;

}

/*
@description      :   关闭/释放设备  
@param - filp     :   要关闭的设备文件 
@return           :   0 成功； 其他 失败
*/
static int led_release(struct inode *inode, struct file *filp)
{
    struct gpioled_dev *dev = filp->private_data;
    //关闭驱动文件时，释放原子变量
    atomic_inc(&dev->lock);
    return 0;
}

/*   设备操作函数集合   */
static struct file_operations gpioled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};


/*
@description      :   驱动入口函数  
@param -          :   无 
@return           :   无
*/
static int __init led_init(void)
{
    int ret;

    atomic_set(&gpioled.lock,1);

    /*  获取设备树中的属性数据  */
    /*  1.获取设备节点：alphaled */
    gpioled.nd = of_find_node_by_path("/light");
    if(gpioled.nd == NULL){
        printk("lightled node can not found!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("lightled node has been found!\r\n");

    }

    /*  2.获取gpio属性内容,得到led使用的gpio编号    */
    gpioled.led_gpio = of_get_named_gpio(gpioled.nd,"light-gpio",0);
    if(gpioled.led_gpio<0)
    {
        printk("can't get led-gpio.");
        return  -EINVAL;
    }
    printk("led-gpio num = %d\r\n",gpioled.led_gpio);

    

    /*   3.设置GPIO1_IO3为输出，并输出高电平，默认关闭led灯   */
    ret = gpio_direction_output(gpioled.led_gpio,1);
    if(ret < 0)
    {
        printk("can't set gpio!\r\n");
    }
    


    //注册字符设备驱动

    //1.创建设备号
    if(gpioled.major)
    {
        gpioled.devid = MKDEV(gpioled.major,0);
        register_chrdev_region(gpioled.devid,GPIOLED_CNT,GPIOLED_NAME);
    }
    else
    {
        alloc_chrdev_region(&gpioled.devid,0,GPIOLED_CNT,GPIOLED_NAME);//申请设备号
        gpioled.major = MAJOR(gpioled.devid);//获取主设备号
        gpioled.minor = MINOR(gpioled.devid);//获取次设备号
    }

    printk("gpioled major=%d,minor=%d\r\n",gpioled.major,gpioled.minor);

    //2.初始化cdev
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev,&gpioled_fops);

    //3.添加cdev
    cdev_add(&gpioled.cdev,gpioled.devid,GPIOLED_CNT);

    //4.创建类
    gpioled.class = class_create(THIS_MODULE,GPIOLED_NAME);
    if(IS_ERR(gpioled.class))
    {
        return PTR_ERR(gpioled.class);
    }
    //5.创建设备
    gpioled.device = device_create(gpioled.class,NULL,gpioled.devid,NULL,GPIOLED_NAME);
    if(IS_ERR(gpioled.device))
    {
        return PTR_ERR(gpioled.device);
    }
    


    return 0;

}

static void __exit led_exit(void)
{


    //注销字符驱动
    cdev_del(&gpioled.cdev);//删除cdev
    unregister_chrdev_region(gpioled.devid,GPIOLED_CNT);
    device_destroy(gpioled.class,gpioled.devid);
    class_destroy(gpioled.class);
    
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Turing");