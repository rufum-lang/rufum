#include "contents.h"

#include <stdio.h>



char *read(char *file_name)
{
    /*
      Open file that contains already processed source code
    */
    FILE *fd;

    fd = fopen(file_name, "r");

    if (fd == NULL)
    {
        perror("Couldn't open input file");

        return NULL;
    }

    /*
      Read contents of that file
      It prints error messages so we son't have to
    */
    char *buffer;

    buffer = get_file_contents(fd);
    fclose(fd);

    return buffer;
}

