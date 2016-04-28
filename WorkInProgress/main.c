#include "utils.h"

char *rootdev = "mydisk";
int main(int argc, char *argv[], char *env[] ) {


    // Path for disk is provided as argument
    if(argv[1])
        rootdev = argv[1];

    init();                     // Initialize and get

    while (1) {

        // Clear the arrays( bzero is defined as a macro in type.h)
        bzero(pathname, 128);
        bzero(params, 64);
        bzero(cmd, 32);
        bzero(line, 1024);

        printf("P[%d] running proc\n",running->pid);
        getInput();
        printf("line = %s\n", line);

        sscanf(line, "%s %s %s", cmd, pathname, params);
        printf("cmd = %s pathname = %s params = %s\n", cmd, pathname, params);

		/*
        if (params[0]) {
            printf("params exists...\n");
            strcat(pathname, " ");
            strcat(pathname, params);
        }

        if(!pathname[0])
        {
            memset(pathname, 0, 128);
            printf("Enter a valid pathname\n");
        } */

        // Arguments are provided with pathname
        printf("full pathname = %s\n", pathname);

        int i = 0;
        while(i < 8)
        {
            if(strcmp(sh_cmds[i], cmd) == 0)
            {
                fptr[i](pathname);          // Execute the function at the index
            }

            i++;
        }

    }



    return 0;
}
