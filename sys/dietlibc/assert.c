#include <stdio.h>

void __assert_fail (const char *__assertion, const char *__file, unsigned int __line, const char *__function)
{
  fprintf(stderr, "Assert faild: %s, file %s, line %d, func %s\n", __assertion, __file, __line, __function);
}
