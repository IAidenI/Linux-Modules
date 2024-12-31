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

MODULE_DESCRIPTION("USB monitor V 0.1 - 31/12/2024");
MODULE_AUTHOR("Aiden");
MODULE_LICENSE("GPL");

struct usb_info {
	int devnum;
	int vendorID;
	int productID;
	struct list_head next;
};

static struct kobject *usb_kobject;
static int is_connected = 0;
static LIST_HEAD(devices);
static int nb_devices = 0;
static DEFINE_MUTEX(data_protected);

/* GESTION DE LA LISTE CHAINEE */
static int usb_device_add(struct usb_device *dev) {
	struct usb_info *node;
	
	node = kmalloc(sizeof(*node), GFP_KERNEL);
	if (!node) {
		printk(KERN_ERR "usb_monitor : Erreur lors de l'allocation mémoire de node.\n");
		return -1;
	}
	
	node->devnum = dev->devnum;
	node->vendorID = dev->descriptor.idVendor;
	node->productID = dev->descriptor.idProduct;
	INIT_LIST_HEAD(&node->next); // Initialisation du pointeur
	list_add_tail(&node->next, &devices); // Ajout à la fin de la liste
	return 0;
}

static void usb_device_del(const int devnum) {
	struct list_head *pos, *q;
	struct usb_info *entry;
	int is_found = 0;
	
	list_for_each_safe(pos, q, &devices) {
		entry = list_entry(pos, struct usb_info, next);
		if (entry->devnum == devnum) {
			list_del(pos);
			kfree(entry);
			is_found = 1;
			break;
		}
	}
	
	if (!is_found) {
		printk(KERN_WARNING "usb_monitor : %d n'as pas été trouvé dans la list.\n", devnum);
	}
}

static void usb_device_clear(void) {
	struct list_head *pos, *q;
	struct usb_info *entry;
	list_for_each_safe(pos, q, &devices) {
		entry = list_entry(pos, struct usb_info, next);
		list_del(pos);
		kfree(entry);
	}
}


/* GESTION DE L'ECRITURE DANS /proc */
static ssize_t info_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	ssize_t len = 0;
	struct usb_info *entry;
	
	mutex_lock(&data_protected);
	len += scnprintf(buf + len, PAGE_SIZE - len, "USB Monitor actif\n");

	list_for_each_entry(entry, &devices, next) {
    		len += scnprintf(buf + len, PAGE_SIZE - len,
        		"\nPériphérique : Vendor ID = %04X ; Product ID = %04X\n",
        		entry->vendorID, entry->productID);
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
	if (!data) {
		printk(KERN_ERR "usb_monitor : Données USB non valides.\n");
		return NOTIFY_BAD;
	}
	struct usb_device *dev = interface_to_usbdev(data);

	if (action == USB_DEVICE_ADD) {
		mutex_lock(&data_protected);
		if (nb_devices < MAX_DEVICES) {
			usb_device_add(dev);
			nb_devices++;
			is_connected = 1;
		} else {
			printk(KERN_WARNING "usb_monitor : Capacité maximal atteinte (%d), aucun autre périphérique ne peut être supporté.\n", MAX_DEVICES);
			mutex_unlock(&data_protected);
			return NOTIFY_BAD;
		}
		mutex_unlock(&data_protected);
	} else if (action == USB_DEVICE_REMOVE) {
		mutex_lock(&data_protected);
		if (nb_devices > 0) {
			usb_device_del(dev->devnum);
			is_connected = 2;
			nb_devices--;
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
	usb_device_clear();
	usb_unregister_notify(&usb);
}

module_init(USB_Monitor_init);
module_exit(USB_Monitor_exit);
