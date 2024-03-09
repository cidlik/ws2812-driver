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
static struct cdev cdevice;
static struct class *dev_class;
static struct spi_device *ws2812_spi;

static struct ws2812_dev_struct {
	struct spi_device *spi_dev;
	int strip_len;
	void *buffer;
} ws2812_dev;

static ssize_t strip_len_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	ws2812_debug(WS2812_LOG_PREFIX "strip_len_show()\n");
	return scnprintf(buf, PAGE_SIZE, "%d\n", ws2812_dev.strip_len);
}

static ssize_t strip_len_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf,
			       size_t count)
{
	int val;

	ws2812_debug(WS2812_LOG_PREFIX "strip_len_store()\n");
	if (kstrtoint(buf, 10, &val) < 0) {
		return -EINVAL;
	}
	ws2812_dev.strip_len = val;
	return count;
}

static inline void ws2812_reset(void)
{
	/* BUG: Reset works just on second call */
	int i, led;
	char buf[24];
	
	for (i = 0; i < 24; i++){
		buf[i] = WS2812_CODE0;
	}
	for (led = 0; led < ws2812_dev.strip_len; led++){
		spi_write(ws2812_spi, (void *)buf, sizeof(buf));
	}
}

static int* hex_to_rgb(char* hex) {
    int r, g, b;
    int* grb; 

    sscanf(hex, "%02x%02x%02x", &r, &g, &b);
    grb = kmalloc(3 * sizeof(int), GFP_KERNEL);
    grb[0] = g;
    grb[1] = r;
    grb[2] = b;
    return grb;
}

static int ws2812_spi_set(int led, char* color)
{
	int bit, c, l, current_bit;
	int digit;
	int* grb = hex_to_rgb(color);
	int buf_len = 3 * 8 * sizeof(char) * ws2812_dev.strip_len;
	char* buf = kzalloc(buf_len, GFP_KERNEL);

	if ((led < 1) || (led > ws2812_dev.strip_len)) {
		ws2812_error("Unsupported value for led: %d\n", led);
		return EINVAL;
	}

	for (l = 0; l < ws2812_dev.strip_len; l++) {
		for (c = 0; c < 3; c++) {
			for (bit = 7; bit >= 0; bit--) {
				current_bit = bit + 8 * c + 24 * l;
#if WS2812_DEBUG_MODE
				ws2812_info("led = %d, color = %d, bit = %d\n",
					    l, c, bit, led);
				ws2812_info("requested led = %d, current_bit = %d\n",
					    led, current_bit);
#endif /* WS2812_DEBUG_MODE */

				if (l + 1 != led) {
					buf[current_bit] = WS2812_CODE0;
					continue;
				}

				digit = grb[c] & (1 << 0);
				grb[c] = grb[c] >> 1;
				if (digit == 1)
					buf[current_bit] = WS2812_CODE1;
				else
					buf[current_bit] = WS2812_CODE0;
			}
		}
	}

	spi_write(ws2812_spi, (void *)buf, buf_len);

	kfree(grb);
	kfree(buf);
	return 0;
}


static int ws2812_command_handler(void)
{
	char *led_num_p;
	char *color_p;
	int led_num;

	if (strstr(ws2812_dev.buffer, "reset")){
		ws2812_reset();
		return 0;
	}

	led_num_p = (char*) ws2812_dev.buffer;
	color_p = strchr(led_num_p, ':');

	if (color_p != NULL) {
		*color_p = '\0';
		ws2812_info("LED number: %s, color code: %s\n",
			    led_num_p, color_p + 1);
		if (kstrtoint(led_num_p, 10, &led_num) < 0) {
			return 1;
		}
		ws2812_spi_set(led_num, color_p + 1);
		return 0;
	}

	return EINVAL;
}

static int ws2812_open(struct inode *, struct file *)
{
	ws2812_debug("ws2812_open()\n");
	return 0;
}

static ssize_t ws2812_write(struct file *, const char __user *chars,
			    size_t size, loff_t *lofft)
{
	int retval;

	ws2812_debug("ws2812_write()\n");
	kfree(ws2812_dev.buffer);
	if (size > WS2812_MEMORY_LIMIT_B) {
		ws2812_error("String too long %d\n", WS2812_MEMORY_LIMIT_B);
		return -1;
	}
	ws2812_dev.buffer = kzalloc(size, GFP_KERNEL);
	if (copy_from_user(ws2812_dev.buffer + *lofft, chars, size)) {
		ws2812_error("Cannot copy data\n");
		return -1;
	}
	*lofft += size;

	retval = ws2812_command_handler();
	if (retval){
		ws2812_error("Unknown command\n");
		return -1;
	}

	return size;
}

static int ws2812_release(struct inode *, struct file *)
{
	ws2812_debug("ws2812_release()\n");
	return 0;
}

static DEVICE_ATTR(strip_len, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
		   strip_len_show, strip_len_store);

static struct attribute *ws2812_attributes[] = {
	&dev_attr_strip_len.attr,
	NULL
};

static const struct attribute_group ws2812_attrs_group = {
	.attrs = ws2812_attributes,
};

static const struct  attribute_group *ws2812_attrs_group_list[] ={
	&ws2812_attrs_group,
	NULL,
};

static struct file_operations f_ops = {
	.owner = THIS_MODULE,
	.open = ws2812_open,
	.write = ws2812_write,
	.release = ws2812_release,
};

static int ws2812_spi_probe(struct spi_device *spi)
{
	ws2812_debug("ws2812_spi_probe()\n");
	ws2812_spi = spi;
	spi->max_speed_hz = 6666666;
	spi->bits_per_word = 8;
	return 0;
}

static void ws2812_spi_remove(struct spi_device *spi)
{
	ws2812_debug("ws2812_spi_remove()\n");
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
	if (retval) {
		ws2812_error("Cannot allocate major number for device\n");
		goto err_exit;
	}
	ws2812_debug("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

	dev_class = class_create(THIS_MODULE, "ws2812_class");
	if (IS_ERR(dev_class)) {
		ws2812_error("Cannot create device class\n");
		goto err_exit;
	}

	if (IS_ERR(device_create_with_groups(dev_class, NULL, dev, NULL,
					     ws2812_attrs_group_list,
					     "ws2812"))) {
		ws2812_error("Cannot create the device\n");
		goto err_exit;
	}
	cdev_init(&cdevice, &f_ops);
	cdev_add(&cdevice, dev, 1);

	if(spi_register_driver(&ws2812_spi_driver)) {
		ws2812_error("Cannot register WS2812 SPI driver\n");
		return EINVAL;
	}
	ws2812_dev.strip_len = WS2812_DEFAULT_STRIP_LEN;

	ws2812_info("Module was inited\n");
	return 0;

err_exit:
	return -1;
}

static void __exit ws2812_exit(void)
{
	ws2812_reset();
	kfree(ws2812_dev.buffer);
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	unregister_chrdev_region(dev, 1);
	spi_unregister_driver(&ws2812_spi_driver);
	ws2812_info("Module exited\n");
}

module_init(ws2812_init);
module_exit(ws2812_exit);

MODULE_AUTHOR("cidlik <zubastikiko@gmail.com>");
MODULE_LICENSE("GPL");
