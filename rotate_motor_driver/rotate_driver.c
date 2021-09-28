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

#define ROTATE_MAJOR 215
#define ROTATE_NAME "ROTATE_DRIVER"
#define GPIO_SIZE 256

char rotate_usage = 0;
static void *rotate_map;
volatile unsigned *rotate;

static int rotate_open(struct inode *minode, struct file *mfile)
{
	if(rotate_usage !=0)
		return -EBUSY;
	rotate_usage = 1;

	rotate_map = ioremap(GPIO_BASE, GPIO_SIZE);
	if(!rotate_map)
	{
		printk("error: mapping gpio memory");
		iounmap(rotate_map);
		return -EBUSY;
	}
	
	rotate = (volatile unsigned int *)rotate_map;
	*(rotate + 1) &= ~(0x7 << (3*0));					// Clear GPIO 10
	*(rotate + 1) |= (0x1 << (3*0));					// Set Output Mode GPIO 10

	return 0;
}

static int rotate_release(struct inode *minode, struct file *mfile)
{
	rotate_usage = 0;
	if(rotate)
		iounmap(rotate);
	return 0;
}

static int rotate_write(struct file *mfile, const char *gdata, size_t length, loff_t *off_what)
{
	char tmp_buf;
	int result;
	int frequency=20000;			// 50HZ for control Servo Motor, 20ms per one frequency time
	int minAngle=600;				// Sign width, when 180 degrees
	int maxAngle=2400;				// Sign width, when 0 degrees
	unsigned int setClock;
	unsigned int clrClock;
	
	result = copy_from_user(&tmp_buf, gdata, length);
	if (result <0)
	{
		printk("Error: copy from user");
		return result;
	}

	printk("data from app: %d \n", tmp_buf);
	
	//control ROTATE
	if(tmp_buf == 3)
	{
		setClock=(minAngle+((maxAngle-minAngle)/180*0));	//0.6 ms, counterclockwise rotation (Speed Phase 3)
		clrClock= frequency-setClock;
		*(rotate+7)=(0x01<<10);						// Output voltage by setClock
		udelay(setClock);
		*(rotate+10)=(0x01<<10);					// 0V by clrClock
		mdelay(clrClock*0.001);
		
		
	}
	else if(tmp_buf == 2)
	{
		
		setClock=(minAngle+((maxAngle-minAngle)/180*67));	// 1.27ms, counterclockwise rotation (Speed Phase 2)
		clrClock= frequency-setClock;
		*(rotate+7)=(0x01<<10);				// Output voltage by setClock
		udelay(setClock);
		*(rotate+10)=(0x01<<10);			// 0V by clrClock
		mdelay(clrClock*0.001);
		
	}
	else if(tmp_buf == 1)
	{
		setClock=(minAngle+((maxAngle-minAngle)/180*76)); // 1.32ms, counterclockwise rotation (Speed Phase 1)
		clrClock=frequency-setClock;
		*(rotate+7)=(0x01<<10);				// Output voltage by setClock
		udelay(setClock);
		*(rotate+10)=(0x01<<10);			// 0V by clrClock
		mdelay(clrClock*0.001);
	}
	else if(tmp_buf == 0)
	{
		setClock=(minAngle+((maxAngle-minAngle)/180*88)); // 1.4ms, Stop (Speed Phase 0)
		clrClock=frequency-setClock;
		*(rotate+7)=(0x01<<10);				// Output voltage by setClock
		udelay(setClock);
		*(rotate+10)=(0x01<<10);			// 0V by clrClock
		mdelay(clrClock*0.001);
	}


	return length;
}

static struct file_operations rotate_fops = 
{
	.owner = THIS_MODULE,
	.open = rotate_open,
	.release = rotate_release,
	.write = rotate_write,
};

static int rotate_init(void)
{
	int result;
	
	result = register_chrdev(ROTATE_MAJOR, ROTATE_NAME, &rotate_fops);
	if(result <0)
	{
		printk(KERN_WARNING "Can't get any major! \n ");
		return result;
	}
	return 0;
}

static void rotate_exit(void)
{
	unregister_chrdev(ROTATE_MAJOR, ROTATE_NAME);
	printk("Motor module removed.\n");
}

module_init(rotate_init);
module_exit(rotate_exit);

MODULE_LICENSE("GPL");
