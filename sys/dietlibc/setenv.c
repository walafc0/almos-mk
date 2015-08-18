#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "dietwarning.h"
link_warning("setenv","setenv calls malloc.  Avoid it in small programs.");

int setenv(const char *name, const char *value, int overwrite) {
  if (getenv(name)) {
    if (!overwrite) return 0;
    unsetenv(name);
  }
  {
    char *c=malloc(strlen(name) + strlen(value) + 2);

    if(c == NULL)
    {
      errno = ENOMEM;
      return -1;
    }

    strcpy(c,name);
    strcat(c,"=");
    strcat(c,value);
    return putenv(c);
  }
}
