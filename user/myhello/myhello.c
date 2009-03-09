#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <asm/nios2.h>
void mydelay(int count)
{
int i;
int j;
j=0;
for(i=0;i<count;i++)
	{
	j=j+i;	
	}
}
int main()
{
//printf("Hello from dxzhang!\n");
//printf("read form switch, write to led:\n");
while(1)
{
	(*na_pio_red_led).np_piodata=(*na_pio_switch).np_piodata;
}
/*
int fd;
int count;
int tmpi;
char buff[4];
if((fd=open("/dev/sw_led",O_RDWR))==-1)
{
perror("open eror");
exit(1);
}
while(1)
{
if(count=read(fd,(char *)buff,4)!=4)
{
perror("read error");
exit(1);
}

mydelay(10000);

if(count=write(fd,(char *)buff,4)!=4)
{
perror("read error");
exit(1);
}

mydelay(10000);


}
*/
}


