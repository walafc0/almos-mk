#include <unistd.h>

int isatty(int fd) {
  return (fd < 4) ? 1 : 0;
}

