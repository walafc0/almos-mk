#ifndef	_CTYPE_H
#define _CTYPE_H

extern int isascii (int c)  __attribute__ ((__const__));
extern int isblank (int c)  __attribute__ ((__const__));
extern int isalnum (int c)  __attribute__ ((__const__));
extern int isalpha (int c)  __attribute__ ((__const__));
extern int isdigit (int c)  __attribute__ ((__const__));
extern int isspace (int c)  __attribute__ ((__const__));

extern int isupper (int c)  __attribute__ ((__const__));
extern int islower (int c)  __attribute__ ((__const__));

extern int toascii(int c)  __attribute__ ((__const__));
extern int tolower(int c)  __attribute__ ((__const__));
extern int toupper(int c)  __attribute__ ((__const__));

extern int isprint(int c)  __attribute__ ((__const__));
extern int ispunct(int c)  __attribute__ ((__const__));
extern int iscntrl(int c)  __attribute__ ((__const__));

/* fscking GNU extensions! */
extern int isxdigit(int c)  __attribute__ ((__const__));

extern int isgraph(int c)  __attribute__ ((__const__));


#endif
