#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"


/*
@description      :   main主程序。
@param - argc     :   argv数组元素个数
@param - argv     :   具体参数  
@return           :   0，成功，其他负值，失败
*/
int main(int argc, char *argv[])
{
    int fd,ret = 0;
    char *filename;
    unsigned char data;

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
        ret  = read( fd, &data, sizeof(data) );
        if( ret < 0 )
        {

        }
        else
        {
            if( data )
                printf("key value = %#X\r\n",data);
        }
        
    }
    


    ret = close(fd);
    if(ret < 0)
    {
        printf("file %s close failed\r\n",argv[1]);
        return -1;
    }
    return 0;   
}