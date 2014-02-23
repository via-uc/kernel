
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

//Needed for cdev creation in /dev
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>

#include <linux/uaccess.h>

#define DRIVER_NAME "hello_world"

struct helloworld_dev
{
	struct cdev cdev;
} *helloworld_devp;

static dev_t helloworld_dev_number;		// The allocated device number
struct class *helloworld_class;			// Tie with the device model

char* hello_world_text;


int helloworld_open(struct inode *inode, struct file *file)
{
	printk(DRIVER_NAME " opening...\n");
	return 0;
}


int helloworld_release(struct inode *inode, struct file *file)
{
	printk(DRIVER_NAME " releasing...\n");
	return 0;
}

ssize_t helloworld_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	printk(DRIVER_NAME " reading...\n");
	if(hello_world_text)
		copy_to_user(buf, hello_world_text, strlen(hello_world_text));

	return hello_world_text ? strlen(hello_world_text) : 0;
}


ssize_t helloworld_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	if(hello_world_text)
		kfree(hello_world_text);

	hello_world_text = kmalloc(count, GFP_KERNEL);
	printk(DRIVER_NAME " writing...\n");
	copy_from_user(hello_world_text, buf, count - 1);
	hello_world_text[count - 1] = '\0';
	printk("Got %zu bytes '%s'\n", count - 1, hello_world_text);
	return count;
}


/* file operations structure - defined in linux/fs.h */
static struct file_operations helloworld_fops =
{
	.owner = THIS_MODULE,
	.open = helloworld_open,
	.release = helloworld_release,
	.read = helloworld_read,
	.write = helloworld_write,
};

// Module init
static int __init helloworld_init(void)
{
	int ret;
	hello_world_text = 0;
	// request dynamic alloc. of device major number (0=start minorno, 1=no of minor devices
	if(alloc_chrdev_region(&helloworld_dev_number,0,1,DRIVER_NAME) < 0)
	{
		printk("can't register device\n");
		return -1;
	}

	// populate sysfs entries
	helloworld_class = class_create(THIS_MODULE, DRIVER_NAME);
	helloworld_devp = kmalloc(sizeof(struct helloworld_dev), GFP_KERNEL);
	if(!helloworld_devp)
		printk("Bad Kmalloc\n");


	// connect file operations to cdev
	cdev_init(&helloworld_devp->cdev,&helloworld_fops);
	helloworld_devp->cdev.owner = THIS_MODULE;

	// connect major/minor number to the cdev
	ret = cdev_add(&helloworld_devp->cdev, (helloworld_dev_number), 1);
	if(ret)
	{
		printk("Bad cdev\n");
		return ret;
	}

	// send uevents to udev so it will create /dev nodes
	device_create(helloworld_class, NULL, MKDEV(MAJOR(helloworld_dev_number), 0), NULL, "helloworld%d", 0);
	printk(DRIVER_NAME " driver initialized.\n");
	return 0;
}


// Module exit
static void __exit helloworld_exit(void)
{
	if(hello_world_text)
		kfree(hello_world_text);
	// release major number
	unregister_chrdev_region((helloworld_dev_number), 1);
	device_destroy(helloworld_class, MKDEV(MAJOR(helloworld_dev_number),0));
	cdev_del(&helloworld_devp->cdev);
	kfree(helloworld_devp);
	//destroy helloworld_class
	class_destroy(helloworld_class);
	printk(DRIVER_NAME " driver exited.\n");
}

MODULE_DESCRIPTION("hello_world module char drivers");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Kiforenko Jensby <martin@rootuser.dk>");

module_init(helloworld_init);
module_exit(helloworld_exit);

