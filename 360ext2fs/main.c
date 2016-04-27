#include <stdio.h>
#include "global.h"
#include "input.h"
#include "functions.h"
#include "init.h"

char *device_name = "mydisk";
char params[128], cmdname[64], line[512];
int main(int argc, char *argv[])
{


    printf("Hello and welcome to 360ext2fs\n");


    // Path for disk is provided as argument
    if(argv[1])
    {
        device_name = argv[1];
    }

    init_fs();
    MountRoot(device_name);

    while (1) {

        // Clear the arrays( bzero is defined as a macro in type.h)
        bzero(pathname, 128);
        bzero(params, 64);
        bzero(cmdname, 32);
        bzero(line, 1024);

        printf("P[%d] running proc\n",running->pid);
        getInput();
        printf("line = %s\n", line);

        sscanf(line, "%s %s %s", cmdname, pathname, params);
        printf("cmd = %s pathname = %s params = %s\n", cmdname, pathname, params);


        /*if (params[0]) {
            printf("params exists...\n");
            strcat(pathname, " ");
            strcat(pathname, params);
        }*/

        if(!pathname[0])
        {
            memset(pathname, 0, 128);
            printf("Enter a valid pathname\n");
        }

        // Arguments are provided with pathname
        printf("full pathname = %s\n", pathname);

        int i = 0;
        while(i < 10)
        {
            if(strcmp(sh_cmds[i], cmdname) == 0)
            {
                fptr[i](pathname);          // Execute the function at the index
            }

            i++;
        }

    }


    return 0;
}
