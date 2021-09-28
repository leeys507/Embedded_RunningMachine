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

#define EXIT_BTN_MAJOR 213
#define EXIT_BTN_NAME "EXIT_BTN_DRIVER"
#define GPIO_SIZE 256
#define BCM_EXIT_BTN 6

char exit_btn_usage = 0;
static void *exit_btn_map;
volatile unsigned *exit_btn;
volatile unsigned exit_btn_check;
static int event_flag = 0;
DECLARE_WAIT_QUEUE_HEAD(exit_btn_waitqueue);

static irqreturn_t exit_btn_itr_handler(int irq,void *pdata)		// exit button interrupt handler
{
	exit_btn_check = (*(exit_btn + 13) & (1 << BCM_EXIT_BTN)) == 0 ? 0 : 1;		// check pin high level
	wake_up_interruptible(&exit_btn_waitqueue);		// process wake up from wait queue
	++event_flag;		// interrupt occurrence indication
	
	return IRQ_HANDLED;
}

static unsigned exit_btn_poll (struct file *mfile, struct poll_table_struct *pt)		// poll wait
{
	int mask = 0;
	
	poll_wait(mfile, &exit_btn_waitqueue, pt);		// process push wait queue

	if(event_flag > 0)		// if interrupt occurrence
		mask |= (POLLIN | POLLRDNORM);		// readable state

	event_flag = 0;			// reset interrupt event
	
	return mask;
}

static int exit_btn_open(struct inode *minode, struct file *mfile)
{
	int r;

	if(exit_btn_usage !=0)
		return -EBUSY;
	exit_btn_usage = 1;

	exit_btn_map = ioremap(GPIO_BASE, GPIO_SIZE);
	if(!exit_btn_map)
	{
		printk("error: mapping gpio memory");
		iounmap(exit_btn_map);
		return -EBUSY;
	}
	
	exit_btn = (volatile unsigned int *)exit_btn_map;
	*(exit_btn) &= ~(0x7 << (3 * BCM_EXIT_BTN));		// clear
	*(exit_btn) |= (0x0 << (3 * BCM_EXIT_BTN));			// input mode

	*(exit_btn + 19) |= (0x1 << BCM_EXIT_BTN);			// rising edge
	
	r = request_irq(gpio_to_irq(BCM_EXIT_BTN), exit_btn_itr_handler, IRQF_TRIGGER_RISING, "gpio_irq_exit_btn", NULL);		// interrupt handler registration
	
	if(r < 0)
	{
		printk("error: request_irq()");
		return r;
	}

	return 0;
}

static int exit_btn_release(struct inode *minode, struct file *mfile)
{
	exit_btn_usage = 0;
	if(exit_btn)
		iounmap(exit_btn);
	free_irq(gpio_to_irq(BCM_EXIT_BTN), NULL);		// remove interrupt handler
	return 0;
}

static int exit_btn_read(struct file *mfile, char *gdata, size_t length, loff_t *off_what)
{
	char rData;
	int result = 0;

	if(exit_btn_check == 1) {		// if push exit switch
		rData = 1;
		result = copy_to_user(gdata, &rData, length);		// data copy to userspace
		exit_btn_check = 0;
	}
	else {
		rData = 0;
		result = copy_to_user(gdata, &rData, length);
	}

	if (result < 0)
	{
		printk("Error: copy to user");
		return result;
	}

	printk("exit_btn data to app: %d \n", rData);
	
	return length;
}

static struct file_operations exit_btn_fops = 
{
	.owner = THIS_MODULE,
	.open = exit_btn_open,
	.release = exit_btn_release,
	.read = exit_btn_read,
	.poll = exit_btn_poll
};

static int exit_btn_init(void)
{
	int result;
	
	result = register_chrdev(EXIT_BTN_MAJOR, EXIT_BTN_NAME, &exit_btn_fops);
	if(result < 0)
	{
		printk(KERN_WARNING "Can't get any major! \n ");
		return result;
	}
	return 0;
}

static void exit_btn_exit(void)
{
	unregister_chrdev(EXIT_BTN_MAJOR, EXIT_BTN_NAME);
	printk("EXIT_BTN module removed.\n");
}

module_init(exit_btn_init);
module_exit(exit_btn_exit);

MODULE_LICENSE("GPL");
