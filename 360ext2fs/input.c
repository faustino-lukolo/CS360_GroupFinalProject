#include "input.h"

extern char line[];


void getInput()
{
    char buf[1024];
    int len = -1;
    char *temp;

    printf("cs360@mysh: ");
    fgets(line, 1024, stdin);
    line[strlen(line) - 1] = 0;     // kill \n
}

char ** tokenize(char *path)
{

    int i = 0;
    char *token, *pathCopy;            // Holds a path name

    // A 2Dimensional Array char *names[256]
    char **pathArr = NULL;
    pathArr = (char **)malloc(sizeof(char *) * 256);

    // preserve the pathname
    pathCopy = strdup(path);

    char *temp;
    pathArr[i++] = strtok(pathCopy, "/");
    while ((pathArr[i] = strtok(0, "/")) != NULL) {
        i++;
    }


    pathArr[i] = 0;
    i =0;

    char *tmp;
    while(pathArr[i])
    {
        tmp = (char*)malloc(sizeof(char)*strlen(pathArr[i]));
        strcpy(tmp, pathArr[i]);
        pathArr[i] = tmp;
        i++;
    }

    return pathArr;
}
