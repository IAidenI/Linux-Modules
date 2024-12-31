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
#define SYS_NAME_CONNECTED "connected"
#define MAX_DEVICES 10
#define MAX_INFO_NAME 128

MODULE_DESCRIPTION("USB monitor V 0.3 - 31/12/2024");
MODULE_AUTHOR("Aiden");
MODULE_LICENSE("GPL");

// Structure pour stocker les informations des périphériques USB
struct usb_info {
	int devnum;
	int vendorID;
	int productID;
	char vendor_name[MAX_INFO_NAME];
	char product_name[MAX_INFO_NAME];
	int usb_version;
	struct list_head next; // Pointeur pour la liste chaînée
};

static struct kobject *usb_kobject; // Kobject pour l'interface sysfs
static int is_connected = 0; // Statut de connexion des périphériques
static LIST_HEAD(devices); // Initialisation de la liste des périphériques
static int nb_devices = 0; // Compteur du nombre de périphériques connectés
static DEFINE_MUTEX(data_protected); // Mutex pour protéger l'accès aux données

/* GESTION DE LA LISTE CHAINEE */
// Fonction pour ajouter un périphérique USB à la liste
static int usb_device_add(struct usb_device *dev) {
	struct usb_info *node;

	node = kmalloc(sizeof(*node), GFP_KERNEL); // Allocation mémoire pour un nouveau nœud
	if (!node) {
		printk(KERN_ERR "usb_monitor : Erreur lors de l'allocation mémoire de node.\n");
		return -1;
	}

	node->devnum = dev->devnum;
	node->vendorID = dev->descriptor.idVendor;
	node->productID = dev->descriptor.idProduct;
	node->usb_version = dev->descriptor.bcdUSB;

	// Récupération du nom du fabricant via les chaînes USB
	int ret = usb_string(dev, dev->descriptor.iManufacturer, node->vendor_name, sizeof(node->vendor_name));
	if (ret < 0) {
		snprintf(node->vendor_name, sizeof(node->vendor_name), "Inconnu");
	}

	// Récupération du nom du produit via les chaînes USB
	ret = usb_string(dev, dev->descriptor.iProduct, node->product_name, sizeof(node->product_name));
	if (ret < 0) {
		snprintf(node->product_name, sizeof(node->product_name), "Inconnu");
	}

	INIT_LIST_HEAD(&node->next); // Initialisation du pointeur suivant
	list_add_tail(&node->next, &devices); // Ajout du nœud à la fin de la liste
	return 0;
}

// Fonction pour supprimer un périphérique USB de la liste
static void usb_device_del(const int devnum) {
	struct list_head *pos, *q;
	struct usb_info *entry;
	int is_found = 0;

	// Parcours sécurisé de la liste pour trouver et supprimer le périphérique
	list_for_each_safe(pos, q, &devices) {
		entry = list_entry(pos, struct usb_info, next);
		if (entry->devnum == devnum) {
			list_del(pos); // Suppression du nœud de la liste
			kfree(entry); // Libération de la mémoire
			is_found = 1;
			break;
		}
	}

	if (!is_found) {
		printk(KERN_WARNING "usb_monitor : %d n'as pas été trouvé dans la list.\n", devnum);
	}
}

// Fonction pour vider complètement la liste des périphériques USB
static void usb_device_clear(void) {
	struct list_head *pos, *q;
	struct usb_info *entry;
	// Parcours sécurisé de la liste pour supprimer tous les nœuds
	list_for_each_safe(pos, q, &devices) {
		entry = list_entry(pos, struct usb_info, next);
		list_del(pos); // Suppression du nœud de la liste
		kfree(entry); // Libération de la mémoire
	}
}


/* GESTION DE L'ECRITURE DANS /proc */
// Fonction pour afficher les informations des périphériques USB via sysfs
static ssize_t info_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	ssize_t len = 0;
	struct usb_info *entry;

	mutex_lock(&data_protected); // Verrouillage du mutex pour accès sécurisé
	len += scnprintf(buf + len, PAGE_SIZE - len, "USB Monitor actif\n");

	// Parcours de la liste des périphériques pour afficher leurs informations
	list_for_each_entry(entry, &devices, next) {
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"\nPériphérique %d :\n"
			"  [ ] Vendor  : ID = %04X - Name = %s\n"
			"  [ ] Product : ID = %04X - Name = %s\n"
			"  [ ] Version USB : %04X\n",
			entry->devnum, entry->vendorID, entry->vendor_name,
			entry->productID, entry->product_name, entry->usb_version);
	}
	mutex_unlock(&data_protected); // Déverrouillage du mutex
	return len;
}

// Fonction pour afficher le statut de connexion via sysfs
static ssize_t connected_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	ssize_t len;
	mutex_lock(&data_protected); // Verrouillage du mutex pour accès sécurisé
	if (is_connected == 1) {
		len = scnprintf(buf, PAGE_SIZE, "CONNECTED\n");
	} else if (is_connected == 2) {
		len = scnprintf(buf, PAGE_SIZE, "DISCONNECTED\n");
	} else {
		len = scnprintf(buf, PAGE_SIZE, "NONE\n");
	}
	is_connected = 0; // Réinitialisation du statut après lecture
	mutex_unlock(&data_protected); // Déverrouillage du mutex
	return len;
}

/* GESTION DU PERIPHERIQUE USB */
// Fonction de gestion des événements USB (ajout/suppression)
static int usb_event(struct notifier_block *self, unsigned long action, void* data) {
	if (!data) {
		printk(KERN_ERR "usb_monitor : Données USB non valides.\n");
		return NOTIFY_BAD;
	}
	struct usb_device *dev = (struct usb_device *)data;

	if (action == USB_DEVICE_ADD) { // Si un périphérique est ajouté
		mutex_lock(&data_protected); // Verrouillage avant modification
		if (nb_devices < MAX_DEVICES) {
			usb_device_add(dev); // Ajout du périphérique à la liste
			nb_devices++;
			is_connected = 1; // Mise à jour du statut de connexion
			// Informe les utilisateurs que SYS_NAME_CONNECTED a été modifié
			sysfs_notify(usb_kobject, NULL, SYS_NAME_CONNECTED);
			printk(KERN_INFO "usb_monitor : Notification sysfs envoyée pour 'connected'.\n");
		} else {
			printk(KERN_WARNING "usb_monitor : Capacité maximale atteinte (%d), aucun autre périphérique ne peut être supporté.\n", MAX_DEVICES);
			mutex_unlock(&data_protected); // Déverrouillage en cas de dépassement
			return NOTIFY_BAD;
		}
		mutex_unlock(&data_protected); // Déverrouillage après modification
	} else if (action == USB_DEVICE_REMOVE) { // Si un périphérique est supprimé
		mutex_lock(&data_protected); // Verrouillage avant modification
		if (nb_devices > 0) {
			usb_device_del(dev->devnum); // Suppression du périphérique de la liste
			is_connected = 2; // Mise à jour du statut de connexion
			nb_devices--;
			// Informe les utilisateurs que SYS_NAME_CONNECTED a été modifié
			sysfs_notify(usb_kobject, NULL, SYS_NAME_CONNECTED); // Correction ici
			printk(KERN_INFO "usb_monitor : Notification sysfs envoyée pour 'connected'.\n");
		}
		mutex_unlock(&data_protected); // Déverrouillage après modification
	}
	return NOTIFY_OK; // Indique que l'événement a été traité
}

static struct notifier_block usb = {
	.notifier_call = usb_event, // Fonction de rappel pour les événements USB
};

/* EQUIVALENT DU MAIN */
// Définition des attributs sysfs pour afficher les informations et le statut de connexion
static struct kobj_attribute info_attribute = __ATTR(info, 0444, info_show, NULL);
static struct kobj_attribute connected_attribute = __ATTR(connected, 0444, connected_show, NULL);

// Fonction d'initialisation du module
static int __init sys_init(void) {
	// Création et ajout du kobject sous /sys/kernel
	usb_kobject = kobject_create_and_add(SYS_NAME, kernel_kobj);
	if (!usb_kobject) {
		return -ENOMEM; // Retourne une erreur si la création échoue
	}

	// Création du fichier sysfs pour les informations USB
	int result = sysfs_create_file(usb_kobject, &info_attribute.attr);
	if (result) {
		kobject_put(usb_kobject); // Nettoyage en cas d'échec
		return result;
	}

	// Création du fichier sysfs pour le statut de connexion
	result = sysfs_create_file(usb_kobject, &connected_attribute.attr);
	if (result) {
		sysfs_remove_file(usb_kobject, &info_attribute.attr); // Suppression en cas d'échec
		kobject_put(usb_kobject);
		return result;
	}
	return 0; // Succès de l'initialisation
}

// Fonction de nettoyage lors du déchargement du module
static void __exit sys_exit(void) {
	sysfs_remove_file(usb_kobject, &info_attribute.attr); // Suppression du fichier info
	sysfs_remove_file(usb_kobject, &connected_attribute.attr); // Suppression du fichier connected
	kobject_put(usb_kobject); // Libération du kobject
}

// Fonction d'initialisation principale du module
static int __init USB_Monitor_init(void) {
	printk(KERN_INFO "usb_monitor : chargé correctement.\n");
	int result = sys_init();
	if (result) {
		printk(KERN_ERR "usb_monitor : Échec lors de l'initialisation sysfs. Code d'erreur : %d\n", result);
		return result;
	}

	usb_register_notify(&usb); // Enregistrement du notifier pour les événements USB
	return 0;
}

// Fonction de nettoyage principale du module
static void __exit USB_Monitor_exit(void) {
	printk(KERN_INFO "usb_monitor : déchargé correctement.\n");
	sys_exit(); // Nettoyage sysfs
	usb_device_clear(); // Vidage de la liste des périphériques
	usb_unregister_notify(&usb); // Désenregistrement du notifier
}

module_init(USB_Monitor_init);
module_exit(USB_Monitor_exit);
