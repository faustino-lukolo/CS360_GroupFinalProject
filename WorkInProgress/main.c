#include "utils.h"

char *rootdev = "mydisk";
char line[1024];
int main(int argc, char *argv[], char *env[] ) {
    /* code */

    init();

    while (1) {
        /* code */
        get_input();
        printf("line = %s\n", line);
    }



    return 0;
}
