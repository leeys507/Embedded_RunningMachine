#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <mach/platform.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

#define MATRIX_MAJOR 227
#define MATRIX_NAME "MATRIX_DRIVER"
#define GPIO_SIZE 256

char matrix_usage = 0;
static void *matrix_map;
volatile unsigned *matrix;

static int matrix_open(struct inode *minode, struct file *mfile)
{
	unsigned char index;

	if(matrix_usage !=0)
		return -EBUSY;
	matrix_usage = 1;

	matrix_map = ioremap(GPIO_BASE, GPIO_SIZE);
	if(!matrix_map)
	{
		printk("error: mapping gpio memory");
		iounmap(matrix_map);
		return -EBUSY;
	}
	
	matrix = (volatile unsigned int *)matrix_map;
	
	for(index = 0; index <= 7; ++index)				
	{
		*(matrix + 2) &= ~(0x07 << (3*index));		// clear GPIO 20 ~ 27
		*(matrix + 2) |= (0x01 << (3*index));		// Set Output Mode GPIO 20 ~ 27
	}

	for(index = 5; index <=9; ++index)			
	{
		*(matrix + 1) &= ~(0x07 << (3*index));		// Clear GPIO 15 ~ 19
		*(matrix + 1) |= (0x01 << (3*index));		// Set Output Mode GPIO 15 ~ 19
	
	}

	return 0;
}

static int matrix_release(struct inode *minode, struct file *mfile)
{
	matrix_usage = 0;
	*(matrix + 10) = (0xff << 20);		// matrix off
	if(matrix)
		iounmap(matrix);
	return 0;
}

static int matrix_write(struct file *mfile, const char *gdata, size_t length, loff_t *off_what)
{
	char tmp_buf[5];
	int i;
	int result;
	char val[5];
	unsigned long delay = jiffies + 1*HZ;			// Delay per 1 second
	
	result = copy_from_user(&tmp_buf, gdata, length);
	if (result <0)
	{
		printk("Error: copy from user");
		return result;
	}

	printk("data from app = %s\n", tmp_buf);


	for(i=0; i<5; i++)
	{
		val[i]=gdata[i];						// Store data from USER area
	}
	while(time_before(jiffies,delay))			// Increase jiffies until become delay
	{

		for(i=0; i<5; i++)
		{
			*(matrix + 7) = (val[i] << 20);		// Exporting data to the vertical line of Dot Matrix	
			*(matrix + 7) = (0x1 << (i+15));	// set 5V on a line by row
			udelay(1);
			*(matrix + 10) = (0xFF << 20);		// Dot Matrix off
			*(matrix + 10) = (0x1F << 15);		// Dot Matrix off
		}
	}

	*(matrix + 10) = (0xFF << 20);
	*(matrix + 10) = (0x1F << 15);
	
	
	return length;
}
		
	
	
static struct file_operations matrix_fops = 
{
	.owner = THIS_MODULE,
	.open = matrix_open,
	.release = matrix_release,
	.write = matrix_write,
};

static int matrix_init(void)
{
	int result;
	
	result = register_chrdev(MATRIX_MAJOR, MATRIX_NAME, &matrix_fops);
	if(result <0)
	{
		printk(KERN_WARNING "Can't get any major! \n ");
		return result;
	}
	return 0;
}

static void matrix_exit(void)
{
	unregister_chrdev(MATRIX_MAJOR, MATRIX_NAME);
	printk("MATRIX module removed.\n");
}

module_init(matrix_init);
module_exit(matrix_exit);

MODULE_LICENSE("GPL");
