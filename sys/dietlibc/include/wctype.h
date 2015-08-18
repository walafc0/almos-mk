#ifndef _WCTYPE_H
#define _WCTYPE_H

#include <wchar.h>

typedef const int32_t* wctrans_t;

int iswalnum(wint_t)  __attribute__ ((__const__));
int iswalpha(wint_t)  __attribute__ ((__const__));
int iswblank(wint_t)  __attribute__ ((__const__));
int iswcntrl(wint_t)  __attribute__ ((__const__));
int iswdigit(wint_t)  __attribute__ ((__const__));
int iswgraph(wint_t)  __attribute__ ((__const__));
int iswlower(wint_t)  __attribute__ ((__const__));
int iswprint(wint_t)  __attribute__ ((__const__));
int iswpunct(wint_t)  __attribute__ ((__const__));
int iswspace(wint_t)  __attribute__ ((__const__));
int iswupper(wint_t)  __attribute__ ((__const__));
int iswxdigit(wint_t)  __attribute__ ((__const__));
int iswctype(wint_t, wctype_t)  __attribute__ ((__const__));
wint_t towctrans(wint_t, wctrans_t) ;
wint_t towlower(wint_t) ;
wint_t towupper(wint_t) ;
wctrans_t wctrans(const char *) ;
wctype_t wctype(const char *) ;

#endif
