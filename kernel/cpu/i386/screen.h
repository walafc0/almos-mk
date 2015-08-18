#ifndef _SCREEN_H_
#define _SCREEN_H_

#define RAMSCREEN   0xB8000	/* debut de la memoire video */
#define SIZESCREEN  0xFA0 	/* 4000, nombres d'octets d'une page texte */
#define SCREENLIM   0xB8FA0

char kX    = 0;                 /* position courante du curseur a l'ecran */
char kY    = 0;
char kattr = 0x02;              /* attributs video des caracteres a afficher */

#include <types.h>

void scrollup (unsigned int);
void putcar   (unsigned char);
void printf   (char *s, ...);
void dump     (uint8_t* addr, int n);

#endif	/* _SCREEN_H_ */
