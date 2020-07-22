#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

#define KEY0VALUE 0xF0
#define INVAKEY   0x00

int main(int argc, char *argv[])
{
    int fd,ret;
    char *filename;
    unsigned char keyvalue;



    if(argc != 2)
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

    while (1)
    {
        read(fd,&keyvalue,sizeof(keyvalue));
        if(keyvalue == KEY0VALUE)
        {
            printf("KEY Press, value = %#X\r\n",keyvalue);
        }
    }
    ret = close(fd);//关闭文件


    if(ret < 0)
    {
        printf("File %s close Failed!\r\n",argv[1]);
        
        return -1;
    }

    return 0;

    
}