#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <mach/platform.h>
#include <linux/io.h>

#define LED_MAJOR 210
#define LED_NAME "LED_SPEED_DRIVER"
#define GPIO_SIZE 256
#define BCM_LED1 2
#define BCM_LED2 3
#define BCM_LED3 4

char led_usage = 0;
static void *led_map;
volatile unsigned *led;

static int led_open(struct inode *minode, struct file *mfile)
{
	if(led_usage !=0)
		return -EBUSY;
	led_usage = 1;

	led_map = ioremap(GPIO_BASE, GPIO_SIZE);
	if(!led_map)
	{
		printk("error: mapping gpio memory");
		iounmap(led_map);
		return -EBUSY;
	}
	
	led = (volatile unsigned int *)led_map;
	*(led) &= ~(0x7 << (3 * BCM_LED1));		// bcm 2 clear
	*(led) |= (0x1 << (3 * BCM_LED1));		// output mode

	*(led) &= ~(0x7 << (3 * BCM_LED2));
	*(led) |= (0x1 << (3 * BCM_LED2));

	*(led) &= ~(0x7 << (3 * BCM_LED3));
	*(led) |= (0x1 << (3 * BCM_LED3));

	return 0;
}

static int led_release(struct inode *minode, struct file *mfile)
{
	led_usage = 0;
	*(led + 10) = (0x1 << BCM_LED1);		// output 0, led off
	*(led + 10) = (0x1 << BCM_LED2);
	*(led + 10) = (0x1 << BCM_LED3);
	if(led)
		iounmap(led);
	return 0;
}

static int led_write(struct file *mfile, const char *gdata, size_t length, loff_t *off_what)
{
	char speed;
	int result;
	
	result = copy_from_user(&speed, gdata, length);
	if (result < 0)
	{
		printk("Error: copy from user");
		return result;
	}

	printk("data from app (speed): %d \n", speed);
	
	//control LED
	if(speed == 0) {						// speed 0
		*(led + 10) = (0x1 << BCM_LED1);	// led off, output 0
		*(led + 10) = (0x1 << BCM_LED2);
		*(led + 10) = (0x1 << BCM_LED3);
	}
	else if(speed == 1) {					// speed 1
		*(led + 7) = (0x1 << BCM_LED1);		// one led on, output 1
		*(led + 10) = (0x1 << BCM_LED2);
		*(led + 10) = (0x1 << BCM_LED3);
	}
	else if(speed == 2) {					// speed 2	
		*(led + 7) = (0x1 << BCM_LED1);		// two led on
		*(led + 7) = (0x1 << BCM_LED2);
		*(led + 10) = (0x1 << BCM_LED3);
	}
	else {									// speed 3
		*(led + 7) = (0x1 << BCM_LED1);		// three led on
		*(led + 7) = (0x1 << BCM_LED2);
		*(led + 7) = (0x1 << BCM_LED3);
	}
	
	return length;
}

static struct file_operations led_fops = 
{
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_release,
	.write = led_write,
};

static int led_init(void)
{
	int result;
	
	result = register_chrdev(LED_MAJOR, LED_NAME, &led_fops);
	if(result < 0)
	{
		printk(KERN_WARNING "Can't get any major! \n ");
		return result;
	}
	return 0;
}

static void led_exit(void)
{
	unregister_chrdev(LED_MAJOR, LED_NAME);
	printk("LED module removed.\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
