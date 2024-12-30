#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#define PROC_NAME "usb_monitor"
#define PROC_NAME_FILE1 "usb_info"
#define PROC_NAME_FILE2 "new_connection"
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
	unsigned long addr;
};

static struct proc_dir_entry *parent_entry, *file1, *file2;
static struct usb_info devices[MAX_DEVICES]; 
static int nb_devices = 0;
static int new_connection = 0;
static DEFINE_MUTEX(usb_mutex);


/* GESTION DU PERIPHERIQUE USB */

/* table of devices that work with this driver */
static struct usb_device_id usb_table [] = {
	{ USB_INTERFACE_INFO(8, 6, 0) }, // Tous les périphériques de stockage
        // USB_DEVICE permet de filtrer par appareil spécifique, par exemple { USB_DEVICE(0x046D, 0xC077) } -> Souris Logitech
        { }                      /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, usb_table);

static int USB_Connection(struct usb_interface *interface, const struct usb_device_id *id) {
	if (nb_devices < MAX_DEVICES) {
		devices[nb_devices].ID = nb_devices;
		devices[nb_devices].vendorID = id->idVendor;
		devices[nb_devices].productID = id->idProduct;
		devices[nb_devices].addr = interface_to_usbdev(interface)->devnum;
		nb_devices++;
		new_connection = 1;
	}
	printk(KERN_INFO "USB Device connected : [Vendor ID = %04X ; Product ID = %04X]\n",
			id->idVendor, id->idProduct);
	return 0;
}

static void USB_Deconnection(struct usb_interface *interface) {
	unsigned long addr = interface_to_usbdev(interface)->devnum;
	
	for (int i = 0; i < nb_devices; i++) {
		if (devices[i].addr == addr) {
			printk(KERN_INFO "USB Device disconnected : [Vendor ID = %04X ; Product ID = %04X]\n",
				devices[i].vendorID, devices[i].productID);
			devices[i] = devices[nb_devices - 1];
			nb_devices--;
			break;
		}
	}
}

static struct usb_driver USBMonitor = {
	.name = "USB monitor",
	.probe = USB_Connection,
	.disconnect = USB_Deconnection,
	.id_table = usb_table,
};

/* GESTION DE L'ECRITURE DANS /PROC */
static int usb_proc_write_device(struct seq_file *m, void *v) {
	mutex_lock(&usb_mutex);
	seq_printf(m, "USB Monitor actif\n");
	seq_printf(m, "    - Appareil connecté : %d\n", nb_devices);
	
	for (int i = 0; i < nb_devices; i++) {
		seq_printf(m, "\nPériphérique %d : Vendor ID = %04X ; Product ID = %04X\n",
				devices[i].ID, devices[i].vendorID, devices[i].productID);
	}
	mutex_unlock(&usb_mutex);
	return 0;
}

static int usb_proc_write_connect(struct seq_file *m, void *v) {
	mutex_lock(&usb_mutex);
	seq_printf(m, "%s\n", new_connection ? "TRUE" : "FALSE");
	new_connection = 0;
	mutex_unlock(&usb_mutex);
	return 0;
}

static int usb_proc_open(struct inode *inode, struct file *file) {
	int file_type = (long)pde_data(inode);
	
	if (file_type == FILE_DEVICE) {
		return single_open(file, usb_proc_write_device, NULL);
	} else if (file_type == FILE_CONNECT) {
		return single_open(file, usb_proc_write_connect, NULL);
	}
	
	return -1;
}

static struct proc_ops usb_proc_fops = {
	.proc_open = usb_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

/* EQUIVALENT DU MAIN */
static int __init proc_init(void) {
	parent_entry = proc_mkdir(PROC_NAME, NULL);	
	if (!parent_entry) {
		return -ENOMEM; // Erreur qui indique qu'il n'y a pas assez de mémoire
	}
	
	file1 = proc_create_data(PROC_NAME_FILE1, 0444, parent_entry, &usb_proc_fops, (void*)FILE_DEVICE);
	if (!file1) {
		remove_proc_entry(PROC_NAME, NULL);
		return -ENOMEM;
	}
	
	file2 = proc_create_data(PROC_NAME_FILE2, 0444, parent_entry, &usb_proc_fops, (void*)FILE_CONNECT);
	if (!file2) {
		remove_proc_entry(PROC_NAME_FILE1, parent_entry);
		remove_proc_entry(PROC_NAME, NULL);
		return -ENOMEM;
	}
	
	return 0;
}

static int __init USB_Monitor_init(void) {
	printk(KERN_INFO "[+] USB Monitor chargé.\n");
	
	int result = proc_init();
	if (result < 0) {
		printk(KERN_ERR "proc_init failed to setup %s. Error number %d\n", PROC_NAME, result);
		return -1;
	}
	
	result = usb_register(&USBMonitor);
	if (result < 0) {
		printk(KERN_ERR "usb_register failed for the %s driver. Error number %d\n",
				USBMonitor.name, result);
		return -1;
	}
	return 0;
}

static void __exit USB_Monitor_exit(void) {
	printk(KERN_INFO "[-] USB Monitor déchargé.\n");
	if (parent_entry) {
		remove_proc_entry(PROC_NAME_FILE1, parent_entry);
		remove_proc_entry(PROC_NAME_FILE2, parent_entry);
		remove_proc_entry(PROC_NAME, NULL);
	}
	usb_deregister(&USBMonitor);
}

module_init(USB_Monitor_init);
module_exit(USB_Monitor_exit);
