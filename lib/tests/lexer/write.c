#include <stdio.h>
#include <string.h>

int write(char *file_name, char *buffer)
{
    /*
      Attempt to open the file
    */
    FILE *fd;

    fd = fopen(file_name, "w");

    if (fd == NULL)
    {
        perror("Failed to open output file");

        return 1;
    }

    /*
      Get size of the buffer excluding terminating NULL byte
    */
    size_t len = strlen(buffer);

    /*
      Write entire buffer (excluding NULL byte) to the file
    */
    fwrite(buffer, len, 1, fd);

    /*
      Check if there were any errors
    */
    if (ferror(fd))
    {
        puts("Encountered I/O error while writing to the output file");
    
        return 1;
    }

    return 0;
}
