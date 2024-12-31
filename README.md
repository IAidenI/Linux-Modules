# Linux-Modules

Ce projet a pour objectif d'explorer le fonctionnement des modules sous Linux. Il propose un programme simple qui affiche un message dans les logs lors du chargement et du déchargement du module.

---

## 📋 Pré-requis

Certaines dépendances doivent être satisfaites pour garantir le bon fonctionnement de ce projet.

### Pour Debian :
```bash
sudo apt install build-essential
sudo apt install linux-headers-$(uname -r)
```

### Pour Arch :
```bash
sudo pacman -S --needed base-devel
sudo pacman -S linux$(uname -r | cut -d '.' -f 1,2 | tr -d .)-headers
```

### Vérification :

Assurez-vous qu'en exécutant la commande suivante :

```bash
ls /lib/modules/$(uname -r)
```

Vous voyez un lien symbolique nommé **`build`** et **`source`**.

---

## 🚀 Hello World !

Voici le programme utilisé pour illustrer le fonctionnement d'un module Linux :

```c
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

MODULE_DESCRIPTION("Hello World du Kernel");
MODULE_AUTHOR("Aiden");
MODULE_LICENSE("GPL");

static int __init Message_init(void) {
        printk(KERN_INFO "Hello world!\n");
        return 0;
}

static void __exit Message_exit(void) {
        printk(KERN_INFO "Goodbye world\n");
}

module_init(Message_init);
module_exit(Message_exit);
```
> L'utilisation de `__init` et `__exit` marque ces fonctions comme utilisables uniquement pendant l'initialisation du module.

---

## 🛠️ Compilation du module

```bash
obj-m=SimpleKernelModule.o
make -C /lib/modules/$(uname -r)/build M=$(pwd)
```

> Ce processus peut être simplifié en utilisant le [`Makefile`](https://github.com/IAidenI/Linux-Modules/tree/main/bin/HelloWorld/Makefile) fourni dans le dépôt.

---

## 📦 Chargement et déchargement du module

```bash
sudo insmod bin/SimpleKernelModule.ko
sudo dmesg | tail -1
[ 4723.035728] Hello world!

sudo rmmod bin/SimpleKernelModule.ko
sudo dmesg | tail -1
[ 4759.100773] Goodbye world
```

---

## 🔑 Keylogger

Ce projet inclut également un **keylogger** plus avancé. Consultez le code complet dans le dépôt.  

Exemple de code :  
```c
MODULE_DESCRIPTION("Keylogger basique");
MODULE_AUTHOR("Aiden");
MODULE_LICENSE("GPL");

// Exemple réduit pour le README
static int __init Keylogger_init(void) {
    printk(KERN_INFO "[+] Keylogger chargé avec succès.\n");
    return 0;
}

static void __exit Keylogger_exit(void) {
    printk(KERN_INFO "[-] Keylogger déchargé avec succès.\n");
}

module_init(Keylogger_init);
module_exit(Keylogger_exit);
```
> Pour un code complet et les keymaps détaillés, consultez [keylogger.c](https://github.com/IAidenI/Linux-Modules/tree/main/bin/Keylogger/keylogger.c).

---

## 🗃️ Persistance des modules (Pour ARCH)

>  Ici je vais prendre l'exemple de [USB Monitor](https://github.com/IAidenI/Linux-Modules/tree/main/bin/USBMonitor/usb_monitor.c) qui est un projet en lien avec [`cette outil`](https://github.com/IAidenI/LinuxTools/blob/main/USBDetector.sh)

Le chargement des modules dans la mémoire vive signifie qu'ils sont supprimés après un redémarrage. Voici comment rendre un module **persistant** :  

### Exemple avec un module nommé `usb_monitor.ko`

1. Créez un répertoire pour stocker les modules personnalisés :  
```bash
sudo mkdir -p /lib/modules/$(uname -r)/kernel/extra/
```

2. Copiez le fichier `.ko` dans ce répertoire :  
```bash
sudo cp usb_monitor.ko /lib/modules/$(uname -r)/kernel/extra/
```

3. Mettez à jour les informations des modules :  
```bash
sudo depmod
```

4. Testez le chargement manuel du module :  
```bash
sudo modprobe usb_monitor
```

Vérifiez que le module est chargé :  
```bash
sudo dmesg | tail -1
[ 6711.317413] usb_monitor : chargé correctement.
```

5. Créez un fichier de configuration pour charger le module au démarrage :  
```bash
sudo nano /etc/modules-load.d/custom-modules.conf
```
Ajoutez le nom du module (sans l'extension `.ko`) :  
```
usb_monitor
```

6. Redémarrez le système :  
```bash
sudo reboot
```

7. Vérifiez que le module est toujours actif après le redémarrage :  
```bash
sudo dmesg | grep usb_monitor
```
