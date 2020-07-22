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
#include <linux/semaphore.h>

#define KEY_CNT      1  //设备号个数
#define KEY_NAME         "key"

#define KEY0VALUE      0xF0   //按键值
#define INVAKEY        0x00   //无效的按键值


//key设备结构体
struct  key_dev
{
    /* data */
    dev_t devid;                //设备号
    struct cdev cdev;           //cdev
    struct class *class;        //类
    struct device *device;      //设备
    int major;                  //主设备号
    int minor;                  //次设备号
    struct  device_node *nd;    //设备节点
    int key_gpio;               //key使用的GPIO编号
    atomic_t keyvalue;          //按键值
};

struct key_dev keydev;//key设备

static int keyio_init(void)
{
    keydev.nd = of_find_node_by_path("/key");
    if(keydev.nd == NULL)
    {
        return -EINVAL;
    }

    keydev.key_gpio = of_get_named_gpio(keydev.nd,"key-gpio",0);
    if(keydev.key_gpio<0)
    {
        printk("can't get key0.\r\n");
        return -EINVAL;
    }
    printk("key_gpio = %d\r\n",keydev.key_gpio);

    //初始化key所用的IO
    gpio_request(keydev.key_gpio,"key0");
    gpio_direction_input(keydev.key_gpio);
    return 0;

}

/*
@description      :   打开设备
@param - inode    :   传递给驱动的inode   
@param - filp     :   设备文件 
@return           :   0 成功； 其他 失败
*/
static int key_open(struct inode *inode, struct file *file)
{
  
    int ret = 0;
    file->private_data = &keydev;        //设置私有数据

    ret = keyio_init();
    if(ret < 0)
    {
        return ret;
    }

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
static ssize_t key_read(struct file *filp, char __user *buf, size_t cnt,loff_t *offt)
{
    int ret = 0;
    unsigned char value;
    struct key_dev *dev = filp->private_data;

    if(gpio_get_value(dev->key_gpio) == 0)//按键被按下
    {
        while (!gpio_get_value(dev->key_gpio));//等待按键被释放
        atomic_set(&dev->keyvalue,KEY0VALUE);
    }
    else
    {
        atomic_set(&dev->keyvalue,INVAKEY);
    }
    value = atomic_read(&dev->keyvalue);//保存按键值
    ret = copy_to_user(buf,&value,sizeof(value));
    return ret;
    
}



/*   设备操作函数集合   */
static struct file_operations key_fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .read = key_read,

};


/*
@description      :   驱动入口函数  
@param -          :   无 
@return           :   无
*/
static int __init mykey_init(void)
{
    //初始化原子变量
    atomic_set(&keydev.keyvalue,INVAKEY);


    //注册字符设备驱动

    //1.创建设备号
    if(keydev.major)
    {
        keydev.devid = MKDEV(keydev.major,0);
        register_chrdev_region(keydev.devid,KEY_CNT,KEY_NAME);
    }
    else
    {
        alloc_chrdev_region(&keydev.devid,0,KEY_CNT,KEY_NAME);//申请设备号
        keydev.major = MAJOR(keydev.devid);//获取主设备号
        keydev.minor = MINOR(keydev.devid);//获取次设备号
    }

    printk("key major=%d,minor=%d\r\n",keydev.major,keydev.minor);

    //2.初始化cdev
    keydev.cdev.owner = THIS_MODULE;
    cdev_init(&keydev.cdev,&key_fops);

    //3.添加cdev
    cdev_add(&keydev.cdev,keydev.devid,KEY_CNT);

    //4.创建类
    keydev.class = class_create(THIS_MODULE,KEY_NAME);
    if(IS_ERR(keydev.class))
    {
        return PTR_ERR(keydev.class);
    }
    //5.创建设备
    keydev.device = device_create(keydev.class,NULL,keydev.devid,NULL,KEY_NAME);
    if(IS_ERR(keydev.device))
    {
        return PTR_ERR(keydev.device);
    }
    


    return 0;

}

static void __exit mykey_exit(void)
{


    //注销字符驱动
    cdev_del(&keydev.cdev);//删除cdev
    unregister_chrdev_region(keydev.devid,KEY_CNT);
    device_destroy(keydev.class,keydev.devid);
    class_destroy(keydev.class);
    
}

module_init(mykey_init);
module_exit(mykey_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Turing");