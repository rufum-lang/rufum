#include "read.h"
#include "scan.h"
#include "write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int dump_file(char *source_file, char *output_file)
{
    /*
      Get textual representation of lunits found in a file
      Internally use rufum_new_file_source()
      It prints error messages so we don't have to
    */
    char *source_buffer = scan_file(source_file);

    if (source_buffer == NULL)
        return 1;

    /*
      Write buffer to a file
    */
    int rv;

    rv =  write(output_file, source_buffer);
    free(source_buffer);

    return rv;
}

static int compare_file(char *source_file, char *input_file, char *output_file)
{
    /*
      Get textual representation of lunits found in a file
      Internally use rufum_new_file_source()
      It prints error messages so we don't have to
    */
    char *source_buffer = scan_file(source_file);

    if (source_buffer == NULL)
        return 1;

    /*
      Read contents of input file so that we can
      compare it with output of scan_file function
    */
    char *input_buffer = read(input_file);

    if (input_buffer == NULL)
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
    */

    write(output_file, source_buffer);
    free(source_buffer);

    return 1;
}

static int dump_string(char *source_file, char *output_file)
{
    /*
      Get textual representation of lunits found in a file
      Internally use rufum_new_string_source()
      It prints error messages so we don't have to
    */
    char *source_buffer = scan_string(source_file);

    if (source_buffer == NULL)
        return 1;

    /*
      Write buffer to a file
    */
    int rv;

    rv =  write(output_file, source_buffer);
    free(source_buffer);

    return rv;
}

static int compare_string(char *source_file, char *input_file, char *output_file)
{
    /*
      Get textual representation of lunits found in a file
      Internally use rufum_new_file_source()
      It prints error messages so we don't have to
    */
    char *source_buffer = scan_string(source_file);

    if (source_buffer == NULL)
        return 1;

    /*
      Read contents of input file so that we can
      compare it with output of scan_file function
    */
    char *input_buffer = read(input_file);

    if (input_buffer == NULL)
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
    */

    write(output_file, source_buffer);
    free(source_buffer);

    return 1;
}

static int file(int argc, char **argv)
{
    if (argc > 5)
    {
        puts("Too many arguments");

        return 1;
    }

    if (argc < 4)
    {
        puts("Too few arguments");

        return 1;
    }

    if (argc == 4)
        return dump_file(argv[2], argv[3]);
    else /* argv == 5 */
        return compare_file(argv[2], argv[3], argv[4]);
}

static int string(int argc, char **argv)
{
    if (argc > 5)
    {
        puts("Too many arguments");

        return 1;
    }

    if (argc < 4)
    {
        puts("Too few arguments");

        return 1;
    }

    if (argc == 4)
        return dump_string(argv[2], argv[3]);
    else /* argc == 5 */
        return compare_string(argv[2], argv[3], argv[4]);
}

int main(int argc, char **argv)
{
    char const * const modes_msg =
        "Mode can be one of:\n"
        "  -f    test lexer with file source\n"
        "  -s    test lexer with string source\n";

    if (argc < 2)
    {
        /*
          An error message, note that puts autamatically appends a newline
          So third line isn't missing '\n'
        */
        fprintf(stderr, "Mode argument missing. %s", modes_msg);

        return 1;
    }

    char *mode;

    mode = argv[1];

    if (strcmp(mode, "-f") == 0)
        return file(argc, argv);

    else if (strcmp(mode, "-s") == 0)
        return string(argc, argv);

    fprintf(stderr, "Bad mode argument. %s", modes_msg);

    return 1;
}
