
#define DEVICE_NAME "ckv"

#include <linux/cdev.h>
// #include "ckv.h"
#include <linux/fs.h>
#include <linux/device.h>


int main(void) {

	static int fw_major;
	struct class *fw_class;
	static struct device* fw_device;
	int accepted_num = 0;
 	int dropped_num = 0;
	static ssize_t fw_device_read(struct file* filp, char __user *buffer, size_t length, loff_t* offset) {
	 	printk("Reading from device, return total number of messages\n");
	 	return sprintf(buffer, "%u", accepted_num + dropped_num);
	}

	static ssize_t fw_device_write(struct file *fp, const char *buff, size_t length, loff_t *ppos)
	{
		printk("Writing to device, clear number of messages\n");
		accepted_num = dropped_num = 0;
		return 0;
	}

	static struct file_operations fops = {
		.read  = fw_device_read,
		.write = fw_device_write
	};

	static int __init fw_module_init(void)
	{
	 	int retval = 0;
		printk("Starting FW module loading\n");

		accepted_num = 0;
		dropped_num = 0;
		// device functions
		fw_major = register_chrdev(0, DEVICE_NAME, &fops);
	 	if (fw_major < 0) {
			printk("failed to register device: error %d\n", fw_major);
			goto failed_chrdevreg;
		}

		fw_class = class_create(THIS_MODULE, DEVICE_NAME);
		if (fw_class == NULL) {
			printk("failed to register device class '%s'\n", DEVICE_NAME);
	 		goto failed_classreg;
		}

	 	fw_device = device_create(fw_class, NULL, MKDEV(fw_major, 0), NULL, DEVICE_FW);
	 	if (fw_device == NULL) {
			printk("failed to create device '%s_%s'\n", DEVICE_NAME, DEVICE_FW);
	 		goto failed_devreg;
		}

		// netfilter functions …
		return 0;
		failed_devreg:
			class_destroy(fw_class);
		// unregister the device class
		failed_classreg:
			unregister_chrdev(fw_major , DEVICE_NAME);
		// remove the device class failed_classreg:
		failed_chrdevreg:
		// unregister the major number failed_chrdevreg:
			return -1;
	}
}
