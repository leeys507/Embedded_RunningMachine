#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <mach/platform.h>
#include <linux/io.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/wait.h>

#define SOUND_MAJOR 212
#define SOUND_NAME "SOUND_SENSOR_DRIVER"
#define GPIO_SIZE 256
#define BCM_SOUND_SENSOR 5

char sound_usage = 0;
static void *sound_map;
volatile unsigned *sound_sensor;
volatile unsigned sound_check;
static int event_flag = 0;
DECLARE_WAIT_QUEUE_HEAD(sound_waitqueue);

static irqreturn_t sound_itr_handler(int irq,void *pdata)		// sound sensor interrupt handler
{
	sound_check = (*(sound_sensor + 13) & (1 << BCM_SOUND_SENSOR)) == 1 ? 0 : 1;		// check pin high level
	wake_up_interruptible(&sound_waitqueue);		// process wake up from wait queue
	++event_flag;		// interrupt occurrence indication
	
	return IRQ_HANDLED;
}

static unsigned sound_poll (struct file *mfile, struct poll_table_struct *pt)		// poll wait
{
	int mask = 0;
	
	poll_wait(mfile, &sound_waitqueue, pt);		// process push wait queue

	if(event_flag > 0)		// if interrupt occurrence
		mask |= (POLLIN | POLLRDNORM);		// readable state

	event_flag =0;		// reset interrupt event
	
	return mask;
}

static int sound_open(struct inode *minode, struct file *mfile)
{
	int r;

	if(sound_usage !=0)
		return -EBUSY;
	sound_usage = 1;

	sound_map = ioremap(GPIO_BASE, GPIO_SIZE);
	if(!sound_map)
	{
		printk("error: mapping gpio memory");
		iounmap(sound_map);
		return -EBUSY;
	}
	
	sound_sensor = (volatile unsigned int *)sound_map;
	*(sound_sensor) &= ~(0x7 << (3 * BCM_SOUND_SENSOR));	// clear
	*(sound_sensor) |= (0x0 << (3 * BCM_SOUND_SENSOR));		// input mode

	*(sound_sensor + 22) |= (0x1 << BCM_SOUND_SENSOR);		// falling edge
	
	r = request_irq(gpio_to_irq(BCM_SOUND_SENSOR), sound_itr_handler, IRQF_TRIGGER_FALLING, "gpio_irq_sound", NULL);		// interrupt handler registration
	
	if(r < 0)
	{
		printk("error: request_irq()");
		return r;
	}

	return 0;
}

static int sound_release(struct inode *minode, struct file *mfile)
{
	sound_usage = 0;
	if(sound_sensor) {
		iounmap(sound_sensor);
	}
	free_irq(gpio_to_irq(BCM_SOUND_SENSOR), NULL);		// remove interrupt handler
	return 0;
}

static int sound_read(struct file *mfile, char *gdata, size_t length, loff_t *off_what)
{
	char rData;
	int result = 0;

	if(sound_check == 1) {		// sound check, if sound sensor output 1
		rData = 1;
		result = copy_to_user(gdata, &rData, length);		// data copy to userspace
		sound_check = 0;
	}
	else {		// no action
		rData = 0;
		result = copy_to_user(gdata, &rData, length);
	}

	if (result < 0)
	{
		printk("Error: copy to user");
		return result;
	}

	printk("sound data to app: %d \n", rData);
	
	return length;
}

static struct file_operations sound_fops = 
{
	.owner = THIS_MODULE,
	.open = sound_open,
	.release = sound_release,
	.read = sound_read,
	.poll = sound_poll
};

static int sound_init(void)
{
	int result;
	
	result = register_chrdev(SOUND_MAJOR, SOUND_NAME, &sound_fops);
	if(result < 0)
	{
		printk(KERN_WARNING "Can't get any major! \n ");
		return result;
	}
	return 0;
}

static void sound_exit(void)
{
	unregister_chrdev(SOUND_MAJOR, SOUND_NAME);
	printk("SOUND module removed.\n");
}

module_init(sound_init);
module_exit(sound_exit);

MODULE_LICENSE("GPL");
