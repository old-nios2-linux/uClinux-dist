//License LGPL
//Author mamba
//This is a program for writing to/reading from nios hardware.
//Also a delay option is there (on my side 5 times slower than real)
//
//Compile it like this - path to uClinux-dist/linux-2.6.x should be adopted 
//nios2-linux-uclibc-gcc -O -s -elf2flt='-s 16000' -I./ -I../uC/uClinux-dist/linux-2.6.x/include -c nios.c
//nios2-linux-uclibc-gcc -O -s -elf2flt='-s 16000' -lm -I./ -I../uC/uClinux-dist/linux-2.6.x/include -o nios nios.o

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/delay.h>

#include <nios2_system.h>    /* Descriptions of standard peripherals */

static void WriteNios(unsigned long addr, unsigned long value);
static unsigned long ReadNios(unsigned long addr);
static void WaitNios(unsigned long us);
void UintToHexStr(unsigned long value, char * hexStr);
int HexStrToUint(char * hexStr, unsigned long *number);
int DecStrToUint(char * decStr, unsigned long *number);

int main(int argc, char **argv, char **envp){
        char y[11];
        strncpy(y, argv[0], 10);
        y[10] = '\0';
        
        int choose = 0;
        int verbose = 0;
        int hex =0;
        if (strcmp(argv[1],"rd")==0){
                choose=1;
        }
        else if (strcmp(argv[1],"rh")==0){
                choose=1;
                hex=1;
        }
        else if (strcmp(argv[1],"Rd")==0){
                choose=1;
                verbose=1;
        }
        else if (strcmp(argv[1],"Rh")==0){
                choose=1;
                verbose=1;
                hex=1;
        }
        else if (strcmp(argv[1],"wd")==0){
                choose=2;
        }
        else if (strcmp(argv[1],"wh")==0){
                choose=2;
                hex=1;
        }
        else if (strcmp(argv[1],"Wd")==0){
                choose=2;
                verbose=1;
        }
        else if (strcmp(argv[1],"Wh")==0){
                choose=2;
                verbose=1;
                hex=1;
        }
        else if (strcmp(argv[1],"dd")==0){
                choose=3;
        }
        else if (strcmp(argv[1],"dh")==0){
                choose=3;
                hex=1;
        }
        else if (strcmp(argv[1],"Dd")==0){
                choose=3;
                verbose=1;
        }
        else if (strcmp(argv[1],"Dh")==0){
                choose=3;
                verbose=1;
                hex=1;
        }
        else{
                printf("nios [type] [address] [value]\n"); 
                printf("[type]\n");  
                printf("rd - read the value from address and return decimal string\n"); 
                printf("wd - write the decimal string of [value] to the address\n");
                printf("Rd - same as parameter rh but with verbose output\n"); 
                printf("Wd - same as parameter wh but with verbose output\n");
                printf("rh - read the value from address and return hex string\n"); 
                printf("wh - write the hex string of [value] to the address\n");
                printf("Rh - same as parameter rh but with verbose output\n"); 
                printf("Wh - same as parameter wh but with verbose output\n");
                printf("dd - wait for micro seconds of decimal value [address]\n");
                printf("Dd - same as parameter dh but with verbose output\n");
                printf("dh - wait for micro seconds of hex value [address]\n");
                printf("Dh - same as parameter dh but with verbose output\n");
                printf("[address]\n"); 
                printf("address (hex) of the component on the avalon bus\n"); 
                printf("[value]\n"); 
                printf("This parameter is only used for writing.\n"); 
                printf("\n");
                printf("Examples:\n"); 
                printf("nios2io wd 80681070 127\n"); 
                printf("nios2io wh 80681070 F\n"); 
                printf("nios2io rd 806810A0 \n"); 
                printf("nios2io rh 806810A0 \n"); 
                printf("nios2io dd 120\n"); 
                printf("nios2io dh ABCDEF\n"); 
                printf("\n");
                return 0;
        }

        if (choose>0){
      
                //printf("choose:%5i, command:%10s, %10s\n", argc, y, argv[1]);
      
                if (choose==1) {//read value
                        char hexStr1[11];
                        strncpy(hexStr1, argv[2], 10);
                        hexStr1[10] = '\0';
	  
                        int ret;
                        unsigned long addr;
                        ret = HexStrToUint(hexStr1, &addr);
                        if (ret==0){
                                if (verbose>0)
                                        printf("Address: %s, %lu\n", hexStr1,addr);
                        } else{
                                printf("Error in hex address!\n");
                                return -1;
                        }
                        //-----------------------------------------------------
                        if (verbose>0)
                                printf("Try to get the contents of address!\n");
                        //-----------------------------------------------------
                        unsigned long value = 0;
                        value = ReadNios(addr);
                        if (hex==0)
                                printf("%lu\n",value);
                        else
                                printf("%08X\n",value);
                } else if (choose==2) {//write value
                        char hexStr1[11];
                        strncpy(hexStr1, argv[2], 10);
                        hexStr1[10] = '\0';
	  
                        int ret;
                        unsigned long addr;
                        ret = HexStrToUint(hexStr1, &addr);
                        if (ret==0){
                                if (verbose>0)
                                        printf("Address: %s, %08X\n", hexStr1,addr);
                        } else {
                                printf("Error in hex address!\n");
                                return -1;
                        }
	  
                        char hexStr2[11];
	  
                        //-----------------------------------------------------
                        if (verbose>0)
                                printf("Writing .... Testing!\n");
                        //-----------------------------------------------------
                        unsigned long val;
                        if (hex==0) {
                                strncpy(hexStr2, argv[3], 10);
                                hexStr2[10] = '\0';
                                ret = DecStrToUint(hexStr2, &val);
                        } else {
                                strncpy(hexStr2, argv[3], 8);
                                hexStr2[8] = '\0';
                                ret = HexStrToUint(hexStr2, &val);
                        }
	  
                        if (ret==0) {
                                if (verbose>0)
                                        printf("Value: %s, %lu\n", hexStr2,val);
                        } else {
                                printf("Error in parameter string!\n");
                                return -1;
                        }
                        //-----------------------------------------------------
                        if (verbose>0)
                                printf("Try to put the value to the address!\n");
                        //-----------------------------------------------------
                        WriteNios(addr, val);
                } else if (choose==3){ //wait us
	
                        char hexStr1[11];
                        
                        int ret;
                        unsigned long us;
                        if (hex==0){
                                strncpy(hexStr1, argv[2], 10);
                                hexStr1[10] = '\0';
                                ret = DecStrToUint(hexStr1, &us);
                        }else {
                                strncpy(hexStr1, argv[2], 8);
                                hexStr1[8] = '\0';
                                ret = HexStrToUint(hexStr1, &us);
                        }
	  
                        if (ret==0){
                                if (verbose>0)
                                        printf("Wait in us: %s, %lu\n", hexStr1,us);
                        }else {
                                printf("Error in hex address!\n");
                                return -1;
                        }
                        //-----------------------------------------------------
                        if (verbose>0)
                                printf("Try to wait some us!\n");
                        //-----------------------------------------------------
                        WaitNios(us);
                } else {//something else
                        //-----------------------------------------------------
                        printf("Testing stuff!\n");
                        //-----------------------------------------------------
                }
        }
        return 0;
}

//convert unsigned 32bit to hex string 
void UintToHexStr(unsigned long value, char * hexStr)
{
        sprintf (hexStr, "%08X",value);
}

//convert hex string to unsigned 32bit 
int HexStrToUint(char * hexStr, unsigned long * number)
{
        char * hexVals="0123456789ABCDEF"; 
        char * hexValsLower="0123456789abcdef";
        int val=0;
        *number=0;
        int i,j;
        for(i=0;i<9;i++) {
                val = -1;
                for(j=0;j<16;j++){
                        if (hexVals[j]==hexStr[i] || hexValsLower[j]==hexStr[i]){
                                val = j;
                                //      printf("HexToUint: Value: %c, %i\n", hexStr[i],val);
                                break;
                        } else if (hexStr[i]=='\0'){
                                //printf("HexToUint:Result: Number: %20s, %10u\n", hexStr,*number);
                                val=-2;
                                break;
                        }
                }
                if (val<0){
                        if (val==-1)
                                return -1;
                        if (val==-2)
                                return 0;//wow everything went fine :-)
                }
                *number=(*number)<<4;
                *number = *number + val;
                //printf("HexToUint: Number%i: %c, %i, %u\n", i, hexStr[i], val ,*number);
        }
        return -1;
}

//convert string to uint
int DecStrToUint(char * decStr, unsigned long *number)
{
        char * decVals="0123456789"; 
        int val=0;
        *number=0;
        int i,j;
        //printf("StrToUint: %20s\n",decStr);
        for(i=0;i<11;i++){
                val = -1;
                for(j=0;j<10;j++){
                        //printf("j:%i- StrToUint: Value: %c, %i\n", j, decStr[i],val);
                        if (decVals[j]==decStr[i]){
                                val = j;
                                //printf("StrToUint: Value: %c, %i\n", decStr[i],val);
                                break;
                        } else if (decStr[i]=='\0'){
                                //printf("StrToUint:Result: Number: %20s, %10u\n", decStr,*number);
                                val=-2;
                                break;
                        }
                }
                if (val<0){
                        //printf("value <0 :%i decStr:%c\n", val, decStr[i]);
                        if (val==-1)
                                return -1;
                        if (val==-2)
                                return 0;//wow everything went fine :-)
                }
                if ( (*number<(unsigned long)429496729)||( (*number==(unsigned long)429496729)&&(val<6)) ){
                        *number=(*number)*10 + val;
                } else{
                        //printf("number too big!\n");
                        return -1;
                }
                //printf("StrToUint: Number%i: %c, %i, %u\n", i, decStr[i], val ,*number);
        }
        return -1;
}

//write 32bit value to avalon bus address
static void WriteNios(unsigned long addr, unsigned long value)
{
        //volatile unsigned long * p;
        //p = (volatile unsigned long *)addr;
        //*p = value;
        (* (volatile unsigned long *)(addr))=value; //all in one line
}
  
//read 32bit value from avalon bus address
static unsigned long ReadNios(unsigned long addr)
{
        //  volatile unsigned long *p;
        //  p = (volatile unsigned long *)addr;
        //  return *p;
        return (unsigned long)(* (volatile unsigned long *)(addr));//all in one line
}

//wait for us
static void WaitNios(unsigned long us)
{
        unsigned long usLoop,i;
        if (us > 2000){
                usLoop=us/2000;
                us = us % usLoop;
                for (i=0;i<usLoop;i++){
                        usleep(2000);
                }
        }   
        usleep(us);
}
