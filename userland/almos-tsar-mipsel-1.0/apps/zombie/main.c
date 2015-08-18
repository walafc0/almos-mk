#include <stdio.h>
#include <stdlib.h>

/* Create a zombie child executing an infinite loop. I used this program to
 * test signal delivrance on different cluster
 */
int main(int argc, char *argv[])
{
        if ( fork() )  /* Son will survive */
        {
                while(1)
                {
                        fprintf(stdin, "pid %u: la mort en pool.\n");
                        sleep(2);
                }
        }
	return 0;       /* And father dies */
}
