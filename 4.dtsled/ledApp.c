#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

#define LEDOFF 0
#define LEDON  1

int main(int argc, char *argv[])
{
    int fd,retvalue;
    char *filename;
    unsigned char datebuf[1];

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

    datebuf[0] = atoi(argv[2]);

    retvalue = write(fd,datebuf,sizeof(datebuf));

    if(retvalue < 0)
    {
        printf("LED Control Failed!\r\n");
        close(fd);
        return -1;
    }

    retvalue = close(fd);
    if(retvalue < 0)
    {
        printf("file %s close Failed!\r\n");
        return -1;
    }
    return 0;

    
}