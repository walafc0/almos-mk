#include <stdio.h>
#include <sys/types.h>

#define MAX_CLSTR 2

int
main(int argc, char **argv, char **envp)
{
        pid_t pid;
        cid_t cid;
        char* args[] = {"/bin/homeless", NULL};

        pid = getpid();
        cid = getcid();

        printf("I'm %d and I'm writing from cluster %d\n", (int)pid, cid );

        /* Kill the father */
        if ( (pid = fork()) )
                return EXIT_SUCCESS;

        /* Force process migration by calling exec() on myself. *
         * Stop when all clusters have been crossed             */
        //if ( cid < MAX_CLSTR )
                execve(args[0], args, envp);

        return EXIT_SUCCESS;
}
