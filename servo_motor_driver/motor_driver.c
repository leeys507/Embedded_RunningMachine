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
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/poll.h>

#define MOTOR_MAJOR 214
#define MOTOR_NAME "MOTOR_DRIVER"
#define GPIO_SIZE 256

char motor_usage = 0;
static void *motor_map;
volatile unsigned *motor;

static int motor_open(struct inode *minode, struct file *mfile)
{
	if(motor_usage !=0)
		return -EBUSY;
	motor_usage = 1;

	motor_map = ioremap(GPIO_BASE, GPIO_SIZE);
	if(!motor_map)
	{
		printk("error: mapping gpio memory");
		iounmap(motor_map);
		return -EBUSY;
	}
	
	motor = (volatile unsigned int *)motor_map;
	*(motor + 0 ) &= ~(0x7 << (3*7));			// Clear GPIO 7
	*(motor + 0 ) |= (0x1 << (3*7));			// Set Output Mode GPIO 7

	*(motor + 0 ) &= ~(0x7 << (3*9));			// Clear GPIO 9
	*(motor + 0 ) |= (0x1 << (3*9));			// Set Output Mode GPIO 9

	*(motor + 0 ) &= ~(0x7 << (3*8));			// Clear GPIO 8
	*(motor + 0 ) |= (0x1 << (3*8));			// Set Output Mode GPIO 8

	*(motor + 1 ) &= ~(0x7 << (3*1));			// Clear GPIO 11
	*(motor + 1 ) |= (0x1 << (3*1));			// Set Output Mode GPIO 11

	return 0;
}

static int motor_release(struct inode *minode, struct file *mfile)
{
	motor_usage = 0;
	if(motor)
		iounmap(motor);
	return 0;
}

static int motor_write(struct file *mfile, const char *gdata, size_t length, loff_t *off_what)
{
	char tmp_buf;
	int result;
	int x;
	
	result = copy_from_user(&tmp_buf, gdata, length);
	if (result < 0)
	{
		printk("Error: copy from user");
		return result;
	}

	printk("data from app: %d \n", tmp_buf);
	
	//control MOTOR
	if(tmp_buf == 1)
	{
		for(x=0;x<=60;x++)						// ClockWise Rotation
		{
			*(motor + 7) = (0x1 << 7);			// GPIO7 - 5v
			*(motor + 10) = (0x1 << 8);			// GPIO8 - 0v
			*(motor + 10) = (0x1 << 9);			// GPIO9 - 0v
			*(motor + 10) = (0x1 << 11);		// GPIO11 - 0v
			mdelay(5);
			
			*(motor + 10) = (0x1 << 7);			// GPIO7 - 0v
			*(motor + 7) = (0x1 << 8);			// GPIO8 - 5v
			*(motor + 10) = (0x1 << 9);			// GPIO9 - 0v
			*(motor + 10) = (0x1 << 11);		// GPIO11 - 0v
			mdelay(5);
			
			*(motor + 10) = (0x1 << 7);			// GPIO7 - 0v
			*(motor + 10) = (0x1 << 8);			// GPIO8 - 0v
			*(motor + 7) = (0x1 << 9);			// GPIO9 - 5v
			*(motor + 10) = (0x1 << 11);		// GPIO11 - 0v
			mdelay(5);

			*(motor + 10) = (0x1 << 7);			// GPIO7 - 0v
			*(motor + 10) = (0x1 << 8);			// GPIO8 - 0v
			*(motor + 10) = (0x1 << 9);			// GPIO9 - 0v
			*(motor + 7) = (0x1 << 11);			// GPIO11 - 5v
			mdelay(5);

		}
	}
	else
	{
		for(x=0;x<=60;x++)						// Counter Clockwise Rotation
		{
			*(motor + 10) = (0x1 << 7);
			*(motor + 10) = (0x1 << 8);
			*(motor + 10) = (0x1 << 9);
			*(motor + 7) = (0x1 << 11);
			mdelay(5);

			*(motor + 10) = (0x1 << 7);
			*(motor + 10) = (0x1 << 8);
			*(motor + 7) = (0x1 << 9);
			*(motor + 10) = (0x1 << 11);
			mdelay(5);

			*(motor + 10) = (0x1 << 7);
			*(motor + 7) = (0x1 << 8);
			*(motor + 10) = (0x1 << 9);
			*(motor + 10) = (0x1 << 11);
			mdelay(5);

			*(motor + 7) = (0x1 << 7);
			*(motor + 10) = (0x1 << 8);
			*(motor + 10) = (0x1 << 9);
			*(motor + 10) = (0x1 << 11);
			mdelay(5);
					
		}
	}
	return length;
}

static struct file_operations motor_fops = 
{
	.owner = THIS_MODULE,
	.open = motor_open,
	.release = motor_release,
	.write = motor_write,
};

static int motor_init(void)
{
	int result;
	
	result = register_chrdev(MOTOR_MAJOR, MOTOR_NAME, &motor_fops);
	if(result <0)
	{
		printk(KERN_WARNING "Can't get any major! \n ");
		return result;
	}
	return 0;
}

static void motor_exit(void)
{
	unregister_chrdev(MOTOR_MAJOR, MOTOR_NAME);
	printk("Motor module removed.\n");
}

module_init(motor_init);
module_exit(motor_exit);

MODULE_LICENSE("GPL");
