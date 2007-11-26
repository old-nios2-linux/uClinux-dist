/*
 *  lcd_16207.h - the header file with the ioctl definitions.
 *
 *  The declarations here have to be in a header file, because
 *  they need to be known both to the kernel module
 *  (in chardev.c) and the process calling ioctl (ioctl.c)
 */

#ifndef LCD_16207_DEV_H
#define LCD_16207_DEV_H

#include <linux/ioctl.h>

/* 
 * The major device number. We can't rely on dynamic 
 * registration any more, because ioctls need to know 
 * it. 
 */
#define MAJOR_NUM 250

/*
 * altera uses a struct for the address to na_lcd_16207_0
 */
#define ADR_LCD_COMMAND na_lcd_16207_0
#define ADR_LCD_READY (na_lcd_16207_0 +4)
#define ADR_LCD_DATA (na_lcd_16207_0 + 8)
#define ADR_LCD_READ (na_lcd_16207_0 + 12)

#define ADR_LCD_LINE1 0x80 + 0x00
#define ADR_LCD_LINE2 0x80 + 0x40

#define BUF_LCD_LINE 16

#define BUF_LCD_ROWS 2

#define BUF_LCD_CHARS BUF_LCD_LINE * BUF_LCD_ROWS

/* 
 * Set the message of the device driver 
 */
#define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, char *)
/*
 * _IOR means that we're creating an ioctl command 
 * number for passing information from a user process
 * to the kernel module. 
 *
 * The first arguments, MAJOR_NUM, is the major device 
 * number we're using.
 *
 * The second argument is the number of the command 
 * (there could be several with different meanings).
 *
 * The third argument is the type we want to get from 
 * the process to the kernel.
 */

/* 
 * Set the waiting time for scrolling up one row
 */
#define IOCTL_SET_DISP_WAIT _IOR(MAJOR_NUM, 1, unsigned long)
/*
 * _IOR means that we're creating an ioctl command 
 * number for passing information from a user process
 * to the kernel module. 
 *
 * The first arguments, MAJOR_NUM, is the major device 
 * number we're using.
 *
 * The second argument is the number of the command 
 * (there could be several with different meanings).
 *
 * The third argument is the type we want to get from 
 * the process to the kernel.
 */


/* 
 * Get the message of the display
 */
#define IOCTL_GET_MSG _IOW(MAJOR_NUM, 2, char *)
/* 
 * This IOCTL is used for output, to get the message 
 * of the device driver. However, we still need the 
 * buffer to place the message in to be input, 
 * as it is allocated by the process.
 */

/* 
 * not working yet -Get the n'th byte of the message 
 */
#define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 3, int)
/* 
 * The IOCTL is used for both input and output. It 
 * receives from the user a number, n, and returns 
 * Message[n]. 
 */

/* 
 * Set the function to the lcd display 
 */
#define IOCTL_EXECUTE_CMD _IOWR(MAJOR_NUM, 3,  *)
/* 
 * The IOCTL is used for both input. It 
 * receives from the user a number, n, and execute a command to
 * the lcd. 
 */

/*
 * Function command codes for io_ctl.
 */
#define LCD_On 1
#define LCD_Off 2
#define LCD_Clear 3
#define LCD_Reset 4
#define LCD_Cursor_Left 5
#define LCD_Cursor_Right 6
#define LCD_Disp_Left 7
#define LCD_Disp_Right 8
#define LCD_Get_Cursor 9
#define LCD_Set_Cursor 10
#define LCD_Home 11
#define LCD_Read 12
#define LCD_Write 13
#define LCD_Cursor_Off 14
#define LCD_Cursor_On 15
#define LCD_Get_Cursor_Pos 16
#define LCD_Set_Cursor_Pos 17
#define LCD_Blink_Off 18

/*
 * Internal LCD signals
 */
#define    LCD_E   0x400 // R/W-signal wired to LCD_RS/LCD_RW
#define    LCD_RS  0x200
#define    LCD_RW  0x100
#define    LCD_DATA  0x0FF

/*
 * LCD Device Commands
 */
#define    LCD_CMD_CLEAR  0x01
#define    LCD_CMD_HOME   0x02
#define    LCD_CMD_MODES  0x04
#define    LCD_CMD_ONOFF  0x08
#define    LCD_CMD_ENABLE_DISP  0x04
#define    LCD_CMD_ENABLE_CURSOR  0x02
#define    LCD_CMD_ENABLE_BLINK  0x01
#define    LCD_CMD_SHIFT  0x10
#define    LCD_CMD_FUNC   0x20
#define    LCD_CMD_RAMCGR 0x40
#define    LCD_CMD_RAMDDR 0x80
#define    LCD_CMD_FILLER 0x00

/*
 * Device Parameters
 */
#define    LCD_INTERFACE   0x10     // interface width;1 (8 BITS)/0 (4BITS)
#define    LCD_NUMROWS     0x08  // # of display rows; 1 (2 ROWS)/0 (1ROW)
#define    LCD_PIXEL       0x04  // character pixel;    1(5x10 dots)/0 (5x7 dot)
#define    LCD_MOVE        0x02  // character move direction; 1(increment)/0(decrement)
#define    LCD_SHIFT       0x01  // move character with display shift; 1(shift) 0(no shift)
#define    LCD_DISP_SHIFT  0x08  // action after character; 1(display shift)/0(character shift)
#define    LCD_COM_RESET 0x30

/*
 * Some LCD parameters
 */
#define LCD_BUS_WIDTH 8
#define LCD_PIXEL_WIDTH 5
#define LCD_PIXEL_HEIGHT 8

/* 
 * The name of the device file 
 */
#define DEVICE_FILE_NAME "/dev/lcd16207"
#define DEVICE_NAME "lcd16207"

/* 
 * The function to write to nios
 */
static void WriteNios(unsigned long addr, unsigned long value);
   
/* 
 * The function to read from nios
 */
static unsigned long ReadNios(unsigned long addr);

/* 
 * The function to wait on nios
 */
static void WaitNios(unsigned long us);

/* 
 * The function to write the buffer to the display
 */
static void LcdWriteLines(void);

/* 
 * The function to read the display to a buffer
 */
static void LcdReadLines(void);

#endif
