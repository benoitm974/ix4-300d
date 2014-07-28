/*
 * wixlcm lcm / lcd driver from gpio - rewritten from iomega GPL sources
 *
 * Author:
 *      Benoit Masson (yahoo@perenite.com)
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>	
#include <linux/uaccess.h>
#include <linux/fs.h>		
#include <linux/miscdevice.h>
#include <linux/mutex.h>

#include <linux/string.h>
#include <asm/string.h>

#include "img.h"
#include "font.h"
#include "initImg.h"

/* pin definition */
#define LCM_BUS_RS                      29      //MPP15 use for RS
#define LCM_BUS_A0                      30      //MPP34 use for A0
#define LCM_BUS_CS                      28      //MPP35 use for CS
#define LCM_BUS_RW              31      //MPP44 use for R/W
#define LCM_BUS_E               32      //MPP45 use for E
#define LCM_DATA_BITS           8

// Data pin mapping
//                                 D0  D1  D2  D3  D4  D5  D6  D7
//--------------------------------------------------------------
static int data_pin_mapping[8] = { 33, 34, 35, 36, 37, 38, 39, 40 };

#define FIRST_PIN 28
#define LAST_PIN 40

#define NBLINES 128
#define NBPAGES	8

#define A0_DAT                          0x01
#define A0_CMD                          0x00
#define PIN_IN                          0x01
#define PIN_OUT                         0x00
#define RS_WRITE                        0x00
#define RS_READ                         0x01

#define INIT_PACKAGE_SIZE 13
unsigned char  initial_lcm_pkg[INIT_PACKAGE_SIZE]=
{
		0xae,   //1
                0xa2,   //2
                0xa0,   //3
                0xc8,   //4
                0xa6,   //5
                0x40,   //6     //0x41
                0x22,   //7
                0x81,   //8
                0x3f,   //9
                0xf8,   //10
                0x00,   //11
                0xa4,   //12
                0x2c,   //13
};


void write_lcm(int A0type,unsigned char tmp)
{
        u32 i=0;

        //set data our TODO: already done by init but to be checked

        //pin E down
        gpio_set_value(LCM_BUS_E, 0x0);

        for ( i=0; i<LCM_DATA_BITS; i++)
        {
                gpio_set_value( data_pin_mapping[i], (tmp&0x01) );
                tmp = tmp >> 1;
        }

        //prepare A0
        gpio_set_value( LCM_BUS_A0, (A0type & 0x01) );
        udelay(30);
        //prepare RW
        gpio_set_value( LCM_BUS_RW, 0x0 );
        gpio_set_value( LCM_BUS_CS, 0x0 );
        //pin E up
        gpio_set_value(LCM_BUS_E, 0x1);
        udelay(100);
        //pin E down
        gpio_set_value(LCM_BUS_E, 0x0);
        gpio_set_value( LCM_BUS_CS, 0x1 );
        udelay(100);

        return;
}

int initialize_lcm(void)
{
        int     i;

        for (i = 0; i< INIT_PACKAGE_SIZE; i++)
                write_lcm(A0_CMD,initial_lcm_pkg[i]);

        mdelay(200);//delay 200ms
        write_lcm(A0_CMD,0x2e);
        mdelay(200);//delay 200ms
        write_lcm(A0_CMD,0x2f);
        mdelay(400);//delay 400ms
        write_lcm(A0_CMD,0xaf);
        return 0;
}

static void clear_lcm(void)
{
        u8 page = 0;
        u8      dat = 0;
        
        write_lcm(A0_CMD,0xae); //display off
         for(page =0 ;page<NBPAGES;page++)
         {
                write_lcm(A0_CMD, page|0xb0);
                write_lcm(A0_CMD, ( (0x0>>4)&0x0f)|0x10 );
                write_lcm(A0_CMD,  (0x0&0x0f) );
                for(dat=0;dat<NBLINES;dat++)
                {
                        write_lcm(A0_DAT, 0 );        
                }        
         }
         write_lcm(A0_CMD,0xaf); //display on    
}

static void draw_lcm(void)
{
        u8 page = 0;
        u8      dat = 0;

        write_lcm(A0_CMD,0xae); //display off
         for(page =0 ;page<NBPAGES;page++)
         {
                write_lcm(A0_CMD, page|0xb0);
                write_lcm(A0_CMD, ( (0x0>>4)&0x0f)|0x10 );
                write_lcm(A0_CMD,  (0x0&0x0f) );
                for(dat=0;dat<NBLINES;dat++)
                {
                        write_lcm(A0_DAT, img[page*NBLINES+dat] );
                }
         }
         write_lcm(A0_CMD,0xaf); //display on
}

static ssize_t wixlcm_read(struct file *file, char __user * out,
			    size_t size, loff_t * off)
{
	ssize_t result;
	int bsize;
	char msg[] = "LCM display usage:\n<code>+<byte array>\ncode: C clear, B draw bmp, T ascii text\n";
	static int buf;
	static int initBuf=1;

	if (initBuf){
		buf = strlen(msg);
		initBuf = 0;
	}
	
	bsize = buf ;	
	if (buf == 0) {
		buf = strlen(msg);
	}
	if (copy_to_user(out, msg, bsize) ){
		result = -EFAULT;
	} else {
		result = bsize;
		buf -= bsize;
	}

	return bsize;
}

static DEFINE_MUTEX(lcm_mutex);

static void draw_init(void)
{
	memcpy(img, initImg, sizeof(img));
        draw_lcm();
}

static ssize_t wixlcm_write(struct file *file, const char __user * in,
			     size_t size, loff_t * off)
{
	u32 c =0, offset, charmemmap;

	unsigned char output[size];
	ssize_t result = size;
	if (size>0){
		/* command switch */
		switch ((char)in[0])	{
	     		case 'C':
				clear_lcm();
				result = size;
			break;
			case 'B':
				//clearing the buffer
				memset(img,0,sizeof(img));

				if (mutex_lock_interruptible(&lcm_mutex)!=0){
					result = -EFAULT;
				} else {
					/* copy the array */
					if (size>NBLINES*NBPAGES+1) {
						result = -EFAULT;
					} else {
						printk(KERN_INFO "/dev/lcm logwrite img: %p size: %d in: %p",img, size, in);
						if (copy_from_user(img, in+1, size-2)==0){
							result = size; 	
						} else {
							result = -EFAULT;
						}
					}
					mutex_unlock(&lcm_mutex);	
					draw_lcm();
				}
			break;
			case 'I':
				draw_init();
			break;
			case 'T':
				//clearing the buffer
                                memset(img,0,sizeof(img));

				strncpy(output, in+1, size-1); output[size-2]='\0';
				for (c=0; c<strlen(output); c++){
					offset = (c*(FONT8_WIDTH+FONT8_SPACE_WIDTH));
					charmemmap = (( ((int)output[c])-FONT8_ASCII_OFFSET)*FONT8_WIDTH);
					printk(KERN_INFO "/dev/lcm charwrite offset: %u memmap: %u char: %u\n", offset, charmemmap, (int)output[c]);
				 	memcpy(img+offset, font8+charmemmap, FONT8_WIDTH);	
				 	memcpy(img+offset+FONT8_WIDTH, font8_space, FONT8_SPACE_WIDTH);
				}
				draw_lcm();
			break;
		}
	}
	strncpy(output, in, size); output[size]='\0';
	printk(KERN_INFO "/dev/lcm written %d : \"%s\"\n", size, output);
	return result;
}

static int wixlcm_open(struct inode *inode, struct file *file)
{
        mutex_init(&lcm_mutex);
        return 0;
}

static int wixlcm_close(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations wixlcm_fops = {
	.owner = THIS_MODULE,
	.open = wixlcm_open,
	.read = wixlcm_read,
	.write = wixlcm_write,
	.release = wixlcm_close,
	.llseek = noop_llseek
};

static struct miscdevice wixlcm_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lcm",
	.fops = &wixlcm_fops
};

/*
 * Module init function
 */
static int __init wixlcm_init(void)
{
        int ret = 0;
	u32 i=0;
        char name[5];

        printk(KERN_INFO "/dev/lcm %s\n", __func__);

        for (i=FIRST_PIN; i<=LAST_PIN; i++) {

        	sprintf(name, "lcm%d", i);
        	// register, turn off
        	ret = gpio_request_one(i, GPIOF_OUT_INIT_LOW, name);

        	if (ret) {
                printk(KERN_ERR "/dev/lcm Unable to request GPIOs: %d\n", ret);
                return ret;
        	}

        }

        //E is already down but should be if init doesn't

        //RS up
        gpio_set_value(LCM_BUS_RS, 0x1);

	//register dev
	misc_register(&wixlcm_misc_device);
	printk(KERN_INFO
	       "/dev/lcm wixlcm device has been registered\n");

	initialize_lcm();
	clear_lcm();
        draw_init();
 
	return ret;
}


/*
 * Module exit function
 */
static void __exit wixlcm_exit(void)
{
	u32 i=0;

        printk(KERN_INFO "/dev/lcm %s\n", __func__);

        for (i=FIRST_PIN; i<=LAST_PIN; i++) {
	        // turn LED off
	        gpio_set_value(i, 0);

	        // unregister GPIO
	        gpio_free(i);

        }

	misc_deregister(&wixlcm_misc_device);
	printk(KERN_INFO "/dev/lcm wixlcm device has been unregistered\n");

}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Benoit Masson");
MODULE_DESCRIPTION("ix4-300d lcd - wixlcm - drive through gpio - rewritten from iomega GPL driver");

module_init(wixlcm_init);
module_exit(wixlcm_exit);
