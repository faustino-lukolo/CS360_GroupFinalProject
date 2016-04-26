#include <stdio.h>
#include <stdlib.h>


char *device_name = "mydisk";
int main(int argc, char *argv[])
{
    printf("Hello and welcome to 360ext2fs\n");
    init_fs();

    if(argv[1])
    {
        device_name = argv[1];
    }

    MountRoot(device_name);


    return 0;
}
