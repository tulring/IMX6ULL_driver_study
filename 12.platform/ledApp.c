#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"


#define LEDOFF 0
#define LEDON 1

/*
@description      :   main主程序。
@param - argc     :   argv数组元素个数
@param - argv     :   具体参数  
@return           :   0，成功，其他负值，失败
*/
int main(int argc, char *argv[])
{
    int fd,ret;
    char *filename;
    unsigned char databuf[2];

    if(argc != 3)
    {
        printf("error usage!\r\n");
        return -1;
    }

    filename = argv[1];

    //打开LED驱动
    fd = open(filename,O_RDWR);

    if(fd<0)
    {
        printf("file %s open failed! \r\n",argv[1]);
        return -1;
    }

    databuf[0] = atoi(argv[2]);//要执行的操作 打开或者关闭
    ret = write(fd,databuf,sizeof(databuf));
    if(ret < 0)
    {
        printf("led control failed!\r\n");
        close(fd);
        return -1;
    }

    ret = close(fd);
    if(ret < 0)
    {
        printf("file %s close failed\r\n",argv[1]);
        return -1;
    }
    return 0;   
}