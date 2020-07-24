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
#include <linux/of_irq.h>
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


#define IMX6UIRQ_CNT                1
#define IMX6UIRQ_NAME           "imx6uirq"
#define KEY0VALUE                       0x01
#define INVAKEY                             0xFF
#define KEY_NUM                          1



//中断IO描述结构体
struct  irq_keydesc
{
    /* data */
    int gpio;                                                                   //gpio
    int irqnum;                                                           //中断编号
    unsigned char value;                                       //按键对应的值
    char name[10];                                                  //名字
    irqreturn_t (*handler)(int,void*);              //终端服务函数

};

struct imx6uirq_dev
{
    /* data */
    dev_t devid;            //设备号
    struct cdev cdev;   //cdev
    struct class *class;    //类
    struct device *device;  //设备
    int major;      //主设备号
    int minor;      //次设备号
    struct device_node *nd;     //设备节点
    atomic_t keyvalue;          //按键值
    atomic_t releasekey;    //标记是否完成一次检测的按键
    struct timer_list timer;        //定时器
    struct irq_keydesc irqkeydesc[KEY_NUM];     //按键描述数组
    unsigned char curkeynum;            //当前的按键号

};

struct imx6uirq_dev imx6uirq;       //irq设备


/*
@description        :   终端服务函数，开启定时器，延时10ms
@param - irq        :   中断号 
@param - dev_id        :   设备结构 
@return             :   中断执行结果
*/
static irqreturn_t key0_handler(int irq,void *dev_id)
{
    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)dev_id;

    dev->curkeynum = 0;
    dev->timer.data = (volatile long)dev_id;
    mod_timer(&dev->timer,jiffies + msecs_to_jiffies(10));//定时10ms
    return IRQ_RETVAL(IRQ_HANDLED);
}

/*
@description        :   定时器服务函数，定时器到了之后，再次读取按键值
@param - arg        :   设备结构变量
@return             :   无
*/
void timer_function(unsigned long arg)
{
    unsigned char value;
    unsigned char num;
    struct irq_keydesc *keydesc;
    struct imx6uirq_dev *dev =  (struct imx6uirq_dev *)arg;

    num = dev->curkeynum;       //获取当前按键编号
    keydesc = &dev->irqkeydesc[num];        //获取当前按键描述
    value = gpio_get_value(keydesc->gpio);      //读取GPIO值
    //按键按下
    if(value == 0)
    {
        atomic_set(&dev->keyvalue,keydesc->value);
    }
    //按键松开
    else
    {
        atomic_set(&dev->keyvalue,0x80|keydesc->value);
        atomic_set(&dev->releasekey,1);
    }
    

}

/*
@description        :   按键IO初始化
@param -                :   无
@return                  :   无
*/
static int keyio_init(void)
{
    unsigned char i = 0;
    int ret = 0;

    //获取设备节点
    imx6uirq.nd = of_find_node_by_path("/key");
    if(imx6uirq.nd == NULL)
    {
        printk("key node not find!\r\n");
        return -EINVAL;
    }

    //获取gpio信息
    for(i = 0; i < KEY_NUM; i++)
    {
        imx6uirq.irqkeydesc[i].gpio = of_get_named_gpio(imx6uirq.nd,"key-gpio",i);
        if(imx6uirq.irqkeydesc[i].gpio < 0)
        {
            printk("can't get key%d\r\n",i);
        }
    }

    for ( i = 0; i < KEY_NUM; i++)
    {
        /* code */
        memset(imx6uirq.irqkeydesc[i].name, 0, sizeof( imx6uirq.irqkeydesc[i].name ) );
        sprintf( imx6uirq.irqkeydesc[i].name, "KEY%d",i );
        gpio_request( imx6uirq.irqkeydesc[i].gpio, imx6uirq.irqkeydesc[i].name  );
        gpio_direction_input(  imx6uirq.irqkeydesc[i].gpio );
        imx6uirq.irqkeydesc[i].irqnum = irq_of_parse_and_map( imx6uirq.nd, i );     //设置中断处理函数

        printk( " key%d:gpio=%d, irqnum = %d\r\n", i, imx6uirq.irqkeydesc[i].gpio, imx6uirq.irqkeydesc[i].irqnum );

    }
    
    imx6uirq.irqkeydesc[0].handler = key0_handler;
    imx6uirq.irqkeydesc[0].value = KEY0VALUE;

    //请求中断
    for ( i = 0; i < KEY_NUM; i++)
    {
        /* code */
        ret = request_irq( imx6uirq.irqkeydesc[i].irqnum, imx6uirq.irqkeydesc[1].handler, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, imx6uirq.irqkeydesc[i].name, &imx6uirq );
        if( ret < 0 )
        {
            printk( "irq %d request failed!\r\n",imx6uirq.irqkeydesc[i].irqnum );
            return -EFAULT;
        }
    }

    //创建定时器
    init_timer(&imx6uirq.timer);
    imx6uirq.timer.function = timer_function;

    return 0;

}


/*
@description        :   打开设备
@param - inode        :   传递给设备的inode    
@param - filp        :   设备文件，file结构体有个叫做private_data的成员变量，一般在open的时候将private_data指向设备结构体  
@return             :   0 成功 其他 失败
*/
static int imx6uirq_open( struct inode *inode, struct file *filp)
{
    filp->private_data = &imx6uirq;     //设置私有数据
    return 0;
}



/*
@description        :   从设备读取数据   
@param - filp        :   设备文件，file结构体有个叫做private_data的成员变量，一般在open的时候将private_data指向设备结构体  
@param - buf        :   返回给用户控件的数据缓冲区
@param - cnt        :   要读取的数据长度
@param - offt        :   相对于文件首地址的偏移
@return             :   0 成功 其他 失败
*/
static ssize_t imx6uirq_read( struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    int ret = 0;
    unsigned char keyvalue = 0;
    unsigned char releasekey = 0;
    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)filp->private_data;

    keyvalue = atomic_read( &dev->keyvalue);
    releasekey = atomic_read( &dev->releasekey );
    if( releasekey )                //有按键按下
    {
        if( keyvalue & 0x080)
        {
            keyvalue &= ~0x80;
            ret = copy_to_user( buf, &keyvalue, sizeof(keyvalue) );

        }
        else
        {
            goto data_error;
        }
        
    }
    return 0;
data_error:
    return -EINVAL;
}

//设备操作函数
static struct file_operations imx6uirq_fops = {
    .owner = THIS_MODULE,
    .open  = imx6uirq_open,
    .read = imx6uirq_read,
};


/*
@description        :   驱动入口函数  
@param                  :   无
@return                   :   0 成功 其他 失败
*/
static int  __init imx6uirq_init( void ) 
{
    //构建设备号
    if( imx6uirq.major )
    {
        imx6uirq.devid = MKDEV( imx6uirq.major, 0 );
        register_chrdev_region( imx6uirq.devid, IMX6UIRQ_CNT, IMX6UIRQ_NAME);

    }
    else
    {
        alloc_chrdev_region( &imx6uirq.devid,0,IMX6UIRQ_CNT,IMX6UIRQ_NAME);
        imx6uirq.major = MAJOR( imx6uirq.devid );
        imx6uirq.minor = MINOR( imx6uirq.devid );
    }

    //注册字符设备
    cdev_init(&imx6uirq.cdev, &imx6uirq_fops);
    cdev_add(&imx6uirq.cdev, imx6uirq.devid, IMX6UIRQ_CNT);

    //创建类
    imx6uirq.class = class_create( THIS_MODULE, IMX6UIRQ_NAME);
    if( IS_ERR( imx6uirq.class ))
    {
        return PTR_ERR( imx6uirq.class);

    }

    //创建设备
    imx6uirq.device = device_create( imx6uirq.class, NULL, imx6uirq.devid, NULL, IMX6UIRQ_NAME);
    if( IS_ERR( imx6uirq.device ))
    {
        return PTR_ERR(imx6uirq.device);
    }
    
    //初始化按键
    atomic_set( &imx6uirq.keyvalue, INVAKEY);
    atomic_set( &imx6uirq.releasekey, 0);
    keyio_init();

    return 0;

}

//驱动出口函数
static void __exit imx6uirq_exit( void )
{
    unsigned int i = 0;

    //删除定时器
    del_timer_sync( &imx6uirq.timer );

    //释放中断
    for ( i = 0; i < KEY_NUM; i++)
    {
        /* code */
        free_irq( imx6uirq.irqkeydesc[i].irqnum, &imx6uirq);
    }
    cdev_del( &imx6uirq.cdev);
    unregister_chrdev_region( imx6uirq.devid, IMX6UIRQ_CNT);
    device_destroy( imx6uirq.class, imx6uirq.devid);
    class_destroy( imx6uirq.class);
}




module_init(imx6uirq_init);
module_exit(imx6uirq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Turing");