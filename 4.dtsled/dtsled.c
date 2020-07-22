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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define DTSLED_CNT      1
#define DTSLED_NAME         "dtsled"

#define LEDOFF      0   //关灯
#define LEDON       1   //开灯




/*   映射后的寄存器虚拟地址   */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO04;
static void __iomem *SW_PAD_GPIO1_IO04;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;


//dtsled设备结构体
struct  dtsled_dev
{
    /* data */
    dev_t devid;                //设备号
    struct cdev cdev;           //cdev
    struct class *class;        //类
    struct device *device;      //设备
    int major;                  //主设备号
    int minor;                  //次设备号
    struct  device_node *nd;    //设备节点
    
};

struct dtsled_dev dtsled;

/*
@description    :   LED打开/关闭
@param - sta    :   LEDON(0)打开LED，LEDOFF(1)关闭LED
@return         :   无
*/
void led_switch(u8 sta)
{
    u32 val = 0;
    if(sta == LEDON)
    {
        val = readl(GPIO1_DR);
        val &= ~(1<<4);
        writel(val,GPIO1_DR);
    }
    else if(sta == LEDOFF)
    {
        /* code */
        val = readl(GPIO1_DR);
        val |= (1<<4);
        writel(val,GPIO1_DR);
    }
    
}


/*
@description      :   打开设备
@param - inode    :   传递给驱动的inode   
@param - filp     :   设备文件 
@return           :   0 成功； 其他 失败
*/
static int led_open(struct inode *inode, struct file *file)
{
    file->private_data = &dtsled;        //设置私有数据
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

    retvalue = copy_from_user(datebuf, buf, cnt);
    if(retvalue < 0)
    {
        printk("kernel write failed!!\r\n");
        return -EFAULT;
    }

    ledstat = datebuf[0];

    if(ledstat  == LEDON)
    {
        led_switch(LEDON);
    }    
    else if (ledstat == LEDOFF)
    {
        led_switch(LEDOFF);
    }
    return 0;

}

/*
@description      :   关闭/释放设备  
@param - filp     :   要关闭的设备文件 
@return           :   0 成功； 其他 失败
*/
static int led_release(struct inode *inode, struct file *file)
{
    return 0;
}

/*   设备操作函数集合   */
static struct file_operations dtsled_fops = {
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
    u32 val = 0;
    u32 regdata[14];
    const char *str;
    struct property *proper;
    /*  获取设备树中的属性数据  */
    /*  1.获取设备节点：alphaled */
    dtsled.nd = of_find_node_by_path("/alphaled");
    if(dtsled.nd == NULL){
        printk("alphaled node can not found!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("alphaled node has been found!\r\n");

    }

    /*  2.获取compatible属性内容    */
    proper = of_find_property(dtsled.nd,"compatible",NULL);
    if(proper == NULL){
        printk("compatible property can not found!\r\n");
        
    }
    else
    {
        printk("compatible = %s\r\n",(char*)proper->value);

    }

    /*   3.获取status属性内容   */
    ret = of_property_read_string(dtsled.nd,"status",&str);
    if(ret < 0)
    {
        printk("status read failed!\r\n");
    }
    else
    {
        printk("status = %s\r\n",str);
    }

    /*  4.获取reg属性内容  */
    ret = of_property_read_u32_array(dtsled.nd,"reg",regdata,10);
    if (ret<0)
    {
        printk("reg property read Failed!\r\n");

    }
    else
    {
        u8 i = 0;
        printk("reg data:\r\n");
        for(i=0;i<10;i++)
        {
            printk("%#X",regdata[i]);
        }
        printk("\r\n");
    }

    
    
    

    /*初始化LED*/

    /*1.寄存器地址映射*/
    
    IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd,0);
    SW_MUX_GPIO1_IO04 = of_iomap(dtsled.nd,1);
    SW_PAD_GPIO1_IO04 = of_iomap(dtsled.nd,2);
    GPIO1_DR = of_iomap(dtsled.nd,3);
    GPIO1_GDIR = of_iomap(dtsled.nd,4);

    /*2.使能GPIO1始终*/
    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26);
    val |= (3 << 26);
    writel(val,IMX6U_CCM_CCGR1);

    /*3.设置GOIO1_IO04 的复用功能，将其复用为GPIO1_IO04，最后设置IO属性*/
    writel(5,SW_MUX_GPIO1_IO04);
    //设置IO属性
    writel(0x10B0,SW_PAD_GPIO1_IO04);
    
    //设置GPIO1_IO04为输出功能
    val = readl(GPIO1_GDIR);
    val &= ~(1<<4);
    val |= (1<<4);
    writel(val,GPIO1_GDIR);

    //默认关闭LED
    val = readl(GPIO1_DR);
    val |= (1<<4);
    writel(val,GPIO1_DR);

    //注册字符设备驱动

    //1.创建设备号
    if(dtsled.major)
    {
        dtsled.devid = MKDEV(dtsled.major,0);
        register_chrdev_region(dtsled.devid,DTSLED_CNT,DTSLED_NAME);
    }
    else
    {
        alloc_chrdev_region(&dtsled.devid,0,DTSLED_CNT,DTSLED_NAME);//申请设备号
        dtsled.major = MAJOR(dtsled.devid);//获取主设备号
        dtsled.minor = MINOR(dtsled.devid);//获取次设备号
    }

    printk("dtsled major=%d,minor=%d\r\n",dtsled.major,dtsled.minor);

    //2.初始化cdev
    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev,&dtsled_fops);

    //3.添加cdev
    cdev_add(&dtsled.cdev,dtsled.devid,DTSLED_CNT);

    //4.创建类
    dtsled.class = class_create(THIS_MODULE,DTSLED_NAME);
    if(IS_ERR(dtsled.class))
    {
        return PTR_ERR(dtsled.class);
    }
    //5.创建设备
    dtsled.device = device_create(dtsled.class,NULL,dtsled.devid,NULL,DTSLED_NAME);
    if(IS_ERR(dtsled.device))
    {
        return PTR_ERR(dtsled.device);
    }
    


    return 0;

}

static void __exit led_exit(void)
{
    //取消映射
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO04);
    iounmap(SW_PAD_GPIO1_IO04);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    //注销字符驱动
    cdev_del(&dtsled.cdev);//删除cdev
    unregister_chrdev_region(dtsled.devid,DTSLED_CNT);
    device_destroy(dtsled.class,dtsled.devid);
    class_destroy(dtsled.class);
    
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Turing");