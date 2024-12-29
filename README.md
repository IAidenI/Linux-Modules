
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
sudo pacman -S linux-headers
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
#include <linux/kernel.h>  // Définitions pour les fonctions liées au noyau.
#include <linux/init.h>    // Macros et définitions pour l'initialisation et la sortie des modules.
#include <linux/module.h>  // Structures et macros pour la gestion des modules du noyau.

MODULE_DESCRIPTION("Hello World du Kernel");  // Description du module.
MODULE_AUTHOR("Aiden");                       // Auteur du module.
MODULE_LICENSE("GPL");                        // Licence (GPL : GNU General Public License).

// Fonction appelée lors du chargement du module dans le noyau.
static int Message_init(void) {
        printk("Hello world!\n");
        return 0;
}

// Fonction appelée lors du déchargement du module du noyau.
static void Message_exit(void) {
        printk("Goodbye world\n");
}

module_init(Message_init);  // Spécifie la fonction d'entrée du module.
module_exit(Message_exit);  // Spécifie la fonction de sortie du module.
```

---

## 🛠️ Compilation du module

Pour compiler ce module, exécutez :

```bash
$ obj-m=SimpleKernelModule.o
$ make -C /lib/modules/$(uname -r)/build M=$(pwd)
```

> Ce processus peut être simplifié en utilisant le [`Makefile`](https://github.com/IAidenI/Linux-Modules/tree/main/bin/Makefile) fourni dans le dépôt.

### Vérification de la compilation :

```bash
$ modinfo bin/SimpleKernelModule.ko
filename:       /home/aiden/Documents/C-C++/Rootkits/Linux-Modules/bin/SimpleKernelModule.ko
license:        GPL
author:         Aiden
description:    Hello World du Kernel
depends:        
retpoline:      Y
name:           SimpleKernelModule
vermagic:       6.1.0-26-amd64 SMP preempt mod_unload modversions
```

---

## 📦 Chargement et déchargement du module

### Chargement dans le noyau :
```bash
$ sudo insmod bin/SimpleKernelModule.ko
$ sudo dmesg | tail -1
[ 4723.035728] Hello world!
```

### Déchargement du module :
```bash
$ sudo rmmod bin/SimpleKernelModule.ko
$ sudo dmesg | tail -1
[ 4759.100773] Goodbye world
``` 
