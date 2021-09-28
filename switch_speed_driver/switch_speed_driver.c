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

#define SWITCH_MAJOR 211
#define SWITCH_NAME "SWITCH_SPEED_DRIVER"
#define GPIO_SIZE 256
#define BCM_SWITCH_SUP 12
#define BCM_SWITCH_SDOWN 13

char switch_usage = 0;
static void *switch_map;
volatile unsigned *switch_btn;
volatile unsigned speed_up, speed_down;
static int event_flag = 0;
DECLARE_WAIT_QUEUE_HEAD(switch_waitqueue);

static irqreturn_t switch_up_itr_handler(int irq,void *pdata)		// speed up switch interrupt handler
{
	speed_up = (*(switch_btn + 13) & (1 << BCM_SWITCH_SUP)) == 0 ? 0 : 1;		// check pin high level
	wake_up_interruptible(&switch_waitqueue);		// process wake up from wait queue
	++event_flag;		// interrupt occurrence indication
	
	return IRQ_HANDLED;
}

static irqreturn_t switch_down_itr_handler(int irq,void *pdata)		// speed down switch interrupt handler
{
	speed_down = (*(switch_btn + 13) & (1 << BCM_SWITCH_SDOWN)) == 0 ? 0 : 1;		// check pin high level
	wake_up_interruptible(&switch_waitqueue);		// process wake up from wait queue
	++event_flag;
	
	return IRQ_HANDLED;
}

static unsigned switch_poll (struct file *mfile, struct poll_table_struct *pt)		// poll wait
{
	int mask = 0;
	
	poll_wait(mfile, &switch_waitqueue, pt);		// process push wait queue

	if(event_flag > 0)		// if interrupt occurrence
		mask |= (POLLIN | POLLRDNORM);		// readable state

	event_flag = 0;		// reset interrupt event
	
	return mask;
}

static int switch_open(struct inode *minode, struct file *mfile)
{
	int r1, r2;
	if(switch_usage != 0)
		return -EBUSY;
	switch_usage = 1;

	switch_map = ioremap(GPIO_BASE, GPIO_SIZE);
	if(!switch_map)
	{
		printk("error: mapping gpio memory");
		iounmap(switch_map);
		return -EBUSY;
	}
	
	switch_btn = (volatile unsigned int *)switch_map;
	*(switch_btn + 1) &= ~(0x7 << (3 * (BCM_SWITCH_SUP % 10)));		// clear
	*(switch_btn + 1) |= (0x0 << (3 * (BCM_SWITCH_SUP % 10)));		// input mode speed up switch

	*(switch_btn + 1) &= ~(0x7 << (3 * (BCM_SWITCH_SDOWN % 10)));
	*(switch_btn + 1) |= (0x0 << (3 * (BCM_SWITCH_SDOWN % 10)));
	
	*(switch_btn + 19) |= (0x1 << BCM_SWITCH_SUP);			// rising edge
	*(switch_btn + 19) |= (0x1 << BCM_SWITCH_SDOWN);
	
	r1 = request_irq(gpio_to_irq(BCM_SWITCH_SUP), switch_up_itr_handler, IRQF_TRIGGER_RISING, "gpio_irq_switch1", NULL);		// speed up switch interrupt handler registration
	r2 = request_irq(gpio_to_irq(BCM_SWITCH_SDOWN), switch_down_itr_handler, IRQF_TRIGGER_RISING, "gpio_irq_switch2", NULL);		// speed down switch interrupt handler registration
	
	if(r1 < 0 || r2 < 0)
	{
		printk("error: request_irq()");
		return r1 < 0 ? r1 : r2;
	}
	
	return 0;
}

static int switch_release(struct inode *minode, struct file *mfile)
{
	switch_usage = 0;
	if(switch_btn)
		iounmap(switch_btn);
	free_irq(gpio_to_irq(BCM_SWITCH_SUP), NULL);		// remove interrupt handler
	free_irq(gpio_to_irq(BCM_SWITCH_SDOWN), NULL);
	return 0;
}

static int switch_read(struct file *mfile, char *gdata, size_t length, loff_t *off_what)
{
	char rData;
	int result = 0;

	if(speed_up == 1) {		// speed up
		rData = 1;
		result = copy_to_user(gdata, &rData, length);
		speed_up = 0;
	}
	else if(speed_down == 1) {	// speed down
		rData = 2;
		result = copy_to_user(gdata, &rData, length);
		speed_down = 0;
	}
	else {		// no action
		rData = 3;
		result = copy_to_user(gdata, &rData, length);
	}

	if (result < 0)
	{
		printk("Error: copy to user");
		return result;
	}

	printk("switch data to app: %d \n", rData);
	
	return length;
}

static struct file_operations switch_fops = 
{
	.owner = THIS_MODULE,
	.open = switch_open,
	.release = switch_release,
	.read = switch_read,
	.poll = switch_poll
};

static int switch_init(void)
{
	int result;
	
	result = register_chrdev(SWITCH_MAJOR, SWITCH_NAME, &switch_fops);
	if(result < 0)
	{
		printk(KERN_WARNING "Can't get any major! \n ");
		return result;
	}
	return 0;
}

static void switch_exit(void)
{
	unregister_chrdev(SWITCH_MAJOR, SWITCH_NAME);
	printk("SWITCH module removed.\n");
}

module_init(switch_init);
module_exit(switch_exit);

MODULE_LICENSE("GPL");
