#include <linux/kernel.h>  // Inclus les définitions pour les fonctions liées au noyau.
#include <linux/init.h>    // Inclus les macros et définitions pour l'initialisation et la sortie des modules.
#include <linux/module.h>  // Inclus les structures et macros pour la gestion des modules du noyau.

MODULE_DESCRIPTION("Hello World du Kernel");  // Description du module.
MODULE_AUTHOR("Aiden");                       // Auteur du module.
MODULE_LICENSE("GPL");                        // Licence du module (GPL : GNU General Public License).

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

