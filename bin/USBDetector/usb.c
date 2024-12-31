#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/list.h>
#include <linux/slab.h>

#define SYS_NAME "usb_monitor"
#define SYS_NAME_INFO "usb_info"
#define SYS_NAME_CONNECTED "is_connected"
#define MAX_DEVICES 10
#define FILE_DEVICE 0
#define FILE_CONNECT 1

MODULE_DESCRIPTION("USB monitor");
MODULE_AUTHOR("Aiden");
MODULE_LICENSE("GPL");

struct usb_info {
	int ID;
	int vendorID;
	int productID;
};

static struct kobject *usb_kobject;
static int is_connected = 0;
static struct usb_info devices[MAX_DEVICES];
static int nb_devices = 0;
static DEFINE_MUTEX(data_protected);


/* GESTION DE L'ECRITURE DANS /proc */
static ssize_t info_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	ssize_t len = 0;
	mutex_lock(&data_protected);
    len += scnprintf(buf + len, PAGE_SIZE - len, "USB Monitor actif\n");

    for (int i = 0; i < nb_devices; i++) {
        len += scnprintf(buf + len, PAGE_SIZE - len, 
            "\nPériphérique %d : Vendor ID = %04X ; Product ID = %04X\n",
            devices[i].ID, devices[i].vendorID, devices[i].productID);
    }
	mutex_unlock(&data_protected);
	return len;
}

static ssize_t connected_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	ssize_t len;
	mutex_lock(&data_protected);
    len = scnprintf(buf, PAGE_SIZE, "%d\n", is_connected);
	is_connected = 0;
	mutex_unlock(&data_protected);
	return len;
}

/* GESTION DU PERIPHERIQUE USB */
static int usb_event(struct notifier_block *self, unsigned long action, void* data) {
	struct usb_device *dev = NULL;
	dev = (struct usb_device *)data;

	if (action == USB_DEVICE_ADD) {
		mutex_lock(&data_protected);
		if (nb_devices < MAX_DEVICES) {
			devices[nb_devices].ID = nb_devices;
			devices[nb_devices].vendorID = dev->descriptor.idVendor;
			devices[nb_devices].productID = dev->descriptor.idProduct;
			nb_devices++;
			is_connected = 1;
		} else {
			printk(KERN_WARNING "usb_monitor : Capacité maximal atteinte (%d), aucun autre périphérique ne peut être supporté.\n", MAX_DEVICES);
		}
		mutex_unlock(&data_protected);
	} else if (action == USB_DEVICE_REMOVE) {
		mutex_lock(&data_protected);
		if (nb_devices > 0) {
			for (int i = 0; i < nb_devices; i++) {
				if (devices[i].vendorID == dev->descriptor.idVendor && devices[i].productID == dev->descriptor.idProduct) {
					for (; i < nb_devices - 1; i++) {
                    	devices[i] = devices[i + 1]; // PROBLEME ICI
                    	devices[i].ID = i;

						is_connected = 2;
						nb_devices--;
						break;
                	}
				}
			}
		}
		mutex_unlock(&data_protected);
	}
	return NOTIFY_OK;
}

static struct notifier_block usb = {
	.notifier_call = usb_event,
};

/* EQUIVALENT DU MAIN */
static struct kobj_attribute info_attribute = __ATTR(info, 0444, info_show, NULL);
static struct kobj_attribute connected_attribute = __ATTR(connected, 0444, connected_show, NULL);

static int __init sys_init(void) {
	usb_kobject = kobject_create_and_add(SYS_NAME, kernel_kobj);
	if (!usb_kobject) {
		return -ENOMEM;
	}

	int result = sysfs_create_file(usb_kobject, &info_attribute.attr);
	if (result) {
		kobject_put(usb_kobject);
		return result;
	}

	result = sysfs_create_file(usb_kobject, &connected_attribute.attr);
	if (result) {
		sysfs_remove_file(usb_kobject, &info_attribute.attr);
		kobject_put(usb_kobject);
		return result;
	}
	return 0;
}

static void __exit sys_exit(void) {
	sysfs_remove_file(usb_kobject, &info_attribute.attr);
	sysfs_remove_file(usb_kobject, &connected_attribute.attr);
	kobject_put(usb_kobject);
}

static int __init USB_Monitor_init(void) {
	printk(KERN_INFO "usb_monitor : chargé correctement.\n");
	int result = sys_init();
    if (result) {
        printk(KERN_ERR "usb_monitor : Échec lors de l'initialisation sysfs. Code d'erreur : %d\n", result);
        return result;
    }

	usb_register_notify(&usb);
	return 0;
}

static void __exit USB_Monitor_exit(void) {
	printk(KERN_INFO "usb_monitor : déchargé correctement.\n");
	sys_exit();
	usb_unregister_notify(&usb);
}

module_init(USB_Monitor_init);
module_exit(USB_Monitor_exit);
