#include "read.h"
#include "scan.h"
#include "write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int dump(char *source_file, char *output_file)
{
    /*
      Get textual representation of lunits found in a file
      It prints error messages so we don't have to
    */
    char *source_buffer = scan_file(source_file);

    if (source_buffer == NULL)
        return 1;

    /*
      Write buffer to a file
      Since write_file is called last an it returns an int
      we can pass its return value to the caller
    */
    int rv;

    rv =  write_file(output_file, source_buffer);
    free(source_buffer);

    return rv;
}

int compare(char *source_file, char *input_file, char *output_file)
{
    /*
      Get textual representation of lunits found in a file
      It prints error messages so we don't have to
    */
    char *source_buffer = scan_file(source_file);

    if (source_buffer == NULL)
        return 1;

    /*
      Read contents of input file so that we can
      compare it with output of scan_file function
    */
    char *input_buffer = read_file(input_file);

    if (input_file == NULL)
    {
        free(source_buffer);

        return 1;
    }

    /*
      Compare two buffers, if they are equal then everything is OK
    */
    if (strcmp(source_buffer, input_buffer) == 0)
    {
        free(source_buffer);
        free(input_buffer);

        return 0;
    }

    free(input_buffer);

    /*
      Contents of source_buffer isn't what we expected
      dump it so that we can analyse the output
      Attempt to open the file for writing, then write
      Since write_file is the last function to be called
      its return value is return value of this function
    */

    write_file(output_file, source_buffer);
    free(source_buffer);

    return 1;
}

int main(int argc, char **argv)
{
    if (argc > 4)
    {
        puts("Too many arguments");

        return 1;
    }

    if (argc < 3)
    {
        puts("Too few arguments");

        return 1;
    }

    if (argc == 3)
        return dump(argv[1], argv[2]);
    else /* argv == 4 */
        return compare(argv[1], argv[2], argv[3]);
}
