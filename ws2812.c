/*
* Sources:
*  * https://embetronicx.com/tutorials/linux/device-drivers/device-file-creation-for-character-drivers/
*/

#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/cdev.h>

#include "ws2812.h"

dev_t dev = 0;
static struct cdev device;
static struct class *dev_class;


static int ws2812_open(struct inode *, struct file *)
{
	pr_info(WS2812_LOG_PREFIX "ws2812_open()\n");
	return 0;
}

static ssize_t ws2812_read(struct file *, char __user *, size_t, loff_t *)
{
	pr_info(WS2812_LOG_PREFIX "ws2812_read()\n");
	return 0;
}

static ssize_t ws2812_write(struct file *, const char __user *, size_t, loff_t *)
{
	pr_info(WS2812_LOG_PREFIX "ws2812_write()\n");
	return 1;
}

static int ws2812_release(struct inode *, struct file *)
{
	pr_info(WS2812_LOG_PREFIX "ws2812_release()\n");
	return 0;
}

static struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.open = ws2812_open,
	.read = ws2812_read,
	.write = ws2812_write,
	.release = ws2812_release,
};

static int __init ws2812_init(void)
{
	int retval;

	retval = alloc_chrdev_region(&dev, 0, 1, "ws2812_region");
	if (retval)	{
		pr_err("Cannot allocate major number for device\n");
		goto err_exit;
	}
	pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

	dev_class = class_create(THIS_MODULE, "ws2812_class");
	if (IS_ERR(dev_class)) {
		pr_err("Cannot create device class\n");
		goto err_exit;
	}

	if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "ws2812"))){
		pr_err("Cannot create the device\n");
		goto err_exit;
	}
	cdev_init(&device, &f_ops);
	cdev_add(&device, dev, 1);

	pr_alert(WS2812_LOG_PREFIX "module was inited\n");
	return 0;

err_exit:
	return -1;
}

static void __exit ws2812_exit(void)
{
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	unregister_chrdev_region(dev, 1);
	pr_alert(WS2812_LOG_PREFIX "module exited\n");
}

module_init(ws2812_init);
module_exit(ws2812_exit);

MODULE_AUTHOR("cidlik <zubastikiko@gmail.com>");
MODULE_LICENSE("GPL");
