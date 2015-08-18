#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MFILE "/etc/shrc"
#define MSIZE 5

int
main(int argc, char** argv)
{
        unsigned int    fd;
        char            buff[MSIZE+1];
        size_t          size;

        fd = open(MFILE, O_CREAT|O_RDONLY, S_IRWXU);

        if ( fd < 0 )
        {
                fprintf(stderr, "I can't open %s !\n",                                  \
                                MFILE);
                return EXIT_FAILURE;
        }

        fprintf(stdin, "%s opened successfully. File descriptor number is %u.\n",       \
                        MFILE, fd);

        size = read(fd, buff, MSIZE);
        buff[MSIZE] = '\0';

        if ( size == -1 )
        {
                fprintf(stderr, "Error while reading !\n");
                return EXIT_FAILURE;
        }

        fprintf(stdin, "I read %u bytes from %s:\t%s\n",                                \
                        size, MFILE, buff);

        if ( close(fd) )
        {
                fprintf(stdin, "I can't close %s !\n",                                  \
                                MFILE);
        }

        return EXIT_SUCCESS;
}
