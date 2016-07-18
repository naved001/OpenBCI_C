
/* Necessary includes for device drivers */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
//#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */
#include <asm/param.h> /* include HZ */
#include <linux/string.h> /* string operations */
#include <linux/timer.h> /* timer gizmos */
#include <linux/list.h> /* include list data struct */
#include <linux/interrupt.h>
#include <linux/random.h>


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Naved Ansari");    
MODULE_DESCRIPTION("Fake OpenBCI board device");  
MODULE_VERSION("1.0");  

/* Declaration of memory.c functions */
static int board_open(struct inode *inode, struct file *filp);
static int board_release(struct inode *inode, struct file *filp);
static ssize_t board_read(struct file *filp,char *buf, size_t count, loff_t *f_pos);
static ssize_t board_write(struct file *filp,const char *buf, size_t count, loff_t *f_pos);
static void board_exit(void);
static int board_init(void);

static void SomeFunction(char d);
static void timer_handler(unsigned long data);

volatile int packetno=0;

/* Structure that declares the usual file */
/* access functions */
struct file_operations board_fops = {
	read: board_read,
	write: board_write,
	open: board_open,
	release: board_release
};

/* Declaration of the init and exit functions */
module_init(board_init);
module_exit(board_exit);

static unsigned capacity = 256;
static unsigned bite = 256;
module_param(capacity, uint, S_IRUGO);
module_param(bite, uint, S_IRUGO);

/* Global variables of the driver */
/* Major number */
static int board_major = 61;

/* Buffer to store data */
static char *board_buffer;
/* length of the current message */
static int board_len;

/* timer pointer */
static struct timer_list the_timer[1];
int timePer = 500;

char command = 's';
char speed = 'H';

static int board_init(void)
{
	int result, ret;
	/* Registering device */
	result = register_chrdev(board_major, "board", &board_fops);
	if (result < 0)
	{
		printk(KERN_ALERT "board: cannot obtain major number %d\n", board_major);
		return result;
	}
	/* Allocating board for the buffer */
	board_buffer = kmalloc(capacity, GFP_KERNEL);
	/* Check if all right */
	if (!board_buffer)
	{ 
		printk(KERN_ALERT "Insufficient kernel memory\n"); 
		result = -ENOMEM;
		goto fail; 
	} 

	memset(board_buffer, 0, capacity);
	board_len = 0;
	printk(KERN_EMERG "Inserting board module\n"); 

	setup_timer(&the_timer[0], timer_handler, 0);
	ret = mod_timer(&the_timer[0], jiffies + msecs_to_jiffies(timePer/2));

	return 0;
	fail: 
	board_exit(); 
	return result;
}

static void board_exit(void)
{
	/* Freeing the major number */
	unregister_chrdev(board_major, "board");
	/* Freeing buffer memory */
	if (board_buffer)
		kfree(board_buffer);
	/* Freeing timer list */
	//if (the_timer)
	//	kfree(the_timer);
	if (timer_pending(&the_timer[0]))
		del_timer(&the_timer[0]);
	printk(KERN_ALERT "Removing board module\n");

}

static int board_open(struct inode *inode, struct file *filp)
{
	/*printk(KERN_INFO "open calMOTOR: process id %d, command %s\n",
		current->pid, current->comm);*/
	/* Success */
	return 0;
}

static int board_release(struct inode *inode, struct file *filp)
{
	/*printk(KERN_INFO "release called: process id %d, command %s\n",
		current->pid, current->comm);*/
	/* Success */
	return 0;
}

static ssize_t board_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	
	if (copy_from_user(board_buffer, buf, 1))
	{
		return -EFAULT;
	}
    
    if(board_buffer[0]== 'b')
    	command = 'b';
    else if(board_buffer[0] == 'v')
    	command = 'v';
    else if (board_buffer[0]=='s')
    {
    	command = 's';
    }
    else
    	command = 'u';

    return count;
}

static ssize_t board_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{ 
	printk(KERN_EMERG "\nIn board_read, command = %c", command); 
	char tbuf[33];
	if(command == 'b')
	{
		//sprintf(tbuf,"\nI will stream data now");
		packetno++;
		//start streaming data, make the packet and then put it on tbuf
		//make fake accel data here. 
		/*float accelArray[3] = {0};
		accelArray[0] = (randomnum(time(NULL)) * 0.1 * (randomnum(time(NULL)) > 0.5 ? -1 : 1)); //x
		accelArray[1] = (randomnum(time(NULL)+2) * 0.1 * (randomnum(time(NULL)+3) > 0.5 ? -1 : 1)); //y
		accelArray[2] = (randomnum(time(NULL)) * 0.4 * (randomnum(time(NULL)) > 0.5 ? -1 : 1)); //z */
		char rand;
		printk(KERN_EMERG "\nIn kernel, starting stream, packet %d", packetno); 
		tbuf[0] = 0xA0;
		int i;
		for(i=1; i<32; i++)
		{
			//tbuf[i] = 0x23;
			get_random_bytes(&rand, sizeof(rand));
			tbuf[i] = rand;
		}
		tbuf[32]= 0xC0;
	}
	else if(command == 'v')
	{
		sprintf(tbuf,"\nBoard has been reset");
		//handle reset
	}
	else if (command == 's')
	{
		sprintf(tbuf,"\n Streaming stopped");
		//handle stop
	}
	else
	{
		sprintf(tbuf, "\nUnknown command");
		//handle unkown condition
	}

	strcpy(board_buffer,tbuf);

	//do not go over then end
	if (count > 256 - *f_pos)
		count = 256 - *f_pos;
	
	/*if (copy_to_user(buf,board_buffer + *f_pos, count))
	{
		return -EFAULT;	
	}*/

	if (copy_to_user(buf,board_buffer, sizeof(tbuf)))
	{
		printk(KERN_ALERT "Couldn't copy to user\n"); 
		return -EFAULT;	

	}
	
	// Changing reading position as best suits 
	*f_pos += count; 
	return count; 
}


static void timer_handler(unsigned long data) 
{
	SomeFunction(command);
	mod_timer(&the_timer[0],jiffies + msecs_to_jiffies(timePer/2));

}	

static void SomeFunction(char d)
{
	//printk("command = %c, speed = %c\n",d,speed);

}




