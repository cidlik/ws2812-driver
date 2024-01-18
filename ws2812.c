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
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/spi/spi.h>

#include "ws2812.h"

dev_t dev = 0;
static struct cdev device;
static struct class *dev_class;
static struct spi_device *ws2812_spi;

void *buffer;


static void ws2812_reset(void)
{
	char buf[24] = {0};
	int i;

	for (i = 0; i < 24; i++){
		buf[i] = WS2812_CODE0;
	}
	spi_write(ws2812_spi, (void *)buf, sizeof(buf));
}


static int ws2812_spi_set(void)
{
	char buf[24] = {0};
	int retval, i;

	for (i = 0; i < 8; i++){
		buf[i] = WS2812_CODE0;
	}
	for (i = 8; i < 24; i++){
		buf[i] = WS2812_CODE1;
	}
	spi_write(ws2812_spi, (void *)buf, sizeof(buf));

	return retval;
}


static int ws2812_on_off(void)
{
	if (strstr(buffer, "on")) {
		pr_info(WS2812_LOG_PREFIX "Turn on LED\n");
		return WS2812_TURN_ON;
	}
	else if (strstr(buffer, "off")) {
		pr_info(WS2812_LOG_PREFIX "Turn on LED\n");
		return WS2812_TURN_OFF;
	}
	else {
		pr_info(WS2812_LOG_PREFIX "NOP\n");
		return EINVAL;
	}
}


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

static ssize_t ws2812_write(struct file *, const char __user *chars, size_t size, loff_t *lofft)
{
	int i;
	int on_off;

	pr_info(WS2812_LOG_PREFIX "ws2812_write()\n");
	pr_info(WS2812_LOG_PREFIX "size: %lu; lofft: %lld\n", size, *lofft);

	kfree(buffer);
	buffer = kzalloc(size, GFP_KERNEL);
	pr_info(WS2812_LOG_PREFIX "Buffer size: %lu\n", sizeof(buffer));
	if (copy_from_user(buffer + *lofft, chars, size)) {
		pr_err(WS2812_LOG_PREFIX "Cannot copy data\n");
		return -1;
	}
	*lofft += size;

	pr_info(WS2812_LOG_PREFIX "Input chars: ");
	for (i = 0; i < size; i++)
		pr_info("%c", *((char *)buffer + (char)i));
	pr_info(WS2812_LOG_PREFIX "\n");

	on_off = ws2812_on_off();
	if (on_off)
		ws2812_spi_set();
	else
		ws2812_reset();

	return size;
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


static int ws2812_spi_probe(struct spi_device *spi)
{
	pr_info("ws2812_spi_probe()\n");
	ws2812_spi = spi;
	spi->max_speed_hz = 6666666;
	spi->bits_per_word = 8;
	return 0;
}


static void ws2812_spi_remove(struct spi_device *spi)
{
	pr_info("ws2812_spi_remove()\n");
}


static const struct of_device_id ws2812_spi_dt_ids[] = {
	{ .compatible = "cidlik,ws2812_spi", },
	{},
};


static const struct spi_device_id ws2812_spi_id_table[] = {
	{"cdk_ws2812_spi", 0},
	{},
};
MODULE_DEVICE_TABLE(spi, ws2812_spi_id_table);


static struct spi_driver ws2812_spi_driver = {
	.probe = ws2812_spi_probe,
	.remove = ws2812_spi_remove,
	.id_table = ws2812_spi_id_table,
	.driver = {
		.name = "cdk_ws2812_spi",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ws2812_spi_dt_ids),
	},
};


static int __init ws2812_init(void)
{
	int retval;

	retval = alloc_chrdev_region(&dev, 0, 1, "ws2812_region");
	if (retval)	{
		pr_err(WS2812_LOG_PREFIX "Cannot allocate major number for device\n");
		goto err_exit;
	}
	pr_info(WS2812_LOG_PREFIX "Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

	dev_class = class_create(THIS_MODULE, "ws2812_class");
	if (IS_ERR(dev_class)) {
		pr_err(WS2812_LOG_PREFIX "Cannot create device class\n");
		goto err_exit;
	}

	if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "ws2812"))){
		pr_err(WS2812_LOG_PREFIX "Cannot create the device\n");
		goto err_exit;
	}
	cdev_init(&device, &f_ops);
	cdev_add(&device, dev, 1);

	if(spi_register_driver(&ws2812_spi_driver)) {
		pr_err(WS2812_LOG_PREFIX "Cannot register WS2812 SPI driver\n");
		return EINVAL;
	}

	pr_alert(WS2812_LOG_PREFIX "Module was inited\n");
	return 0;

err_exit:
	return -1;
}

static void __exit ws2812_exit(void)
{
	kfree(buffer);
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	unregister_chrdev_region(dev, 1);
	spi_unregister_driver(&ws2812_spi_driver);
	pr_alert(WS2812_LOG_PREFIX "Module exited\n");
}

module_init(ws2812_init);
module_exit(ws2812_exit);

MODULE_AUTHOR("cidlik <zubastikiko@gmail.com>");
MODULE_LICENSE("GPL");
