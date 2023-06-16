#include <stdbool.h>

/*
  This function checks for lowercase character
  It is used by other functions below
*/
static inline bool test_char_lowercase(int c)
{
    return c >= 'a' && c <= 'z';
}

/*
  This function checks for uppercase characters
  It is used by other functions below
*/
static inline bool test_char_uppercase(int c)
{
    return c >= 'A' && c <= 'Z';
}

/*
  Check if character is a binary digit
*/
static inline bool test_char_binary(int c)
{
    return c == '0' || c == '1';
}

/*
  Check if character is an octal digit
*/
static inline bool test_char_octal(int c)
{
    return c >= '0' && c <= '7';
}

/*
  Check if character is a decimal digit
*/
static inline bool test_char_decimal(int c)
{
    return c >= '0' && c <= '9';
}

/*
  Check if character is a hexadecimal digit
*/
static inline bool test_char_hexadecimal(int c)
{
    bool rv;

    rv = test_char_decimal(c);
    rv = rv || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');

    return rv;
}

/*
  A suffix is a string of characters that follow a number but that make
  the number invalid. Example: in octal system 8 marks beginning of a suffix
  because 8 isn't a valid octal digit. Any characters that can form a number
  in a given numeral system with the exclusion of characters that can form an
  identifier (see test_char_following) gives us characters that can begin a
  suffix. Example:
    identifier characters: a-z A-Z 0-9 ? _
    octal characters: 0-7 , .
    identifier \ octal (set difference): a-z A-Z 8 9 ? _

  This function checks if character can begin a suffix of binary number
*/
static inline bool test_char_binary_suffix(int c)
{
    bool rv;

    rv = test_char_lowercase(c) || test_char_uppercase(c);
    rv = rv || (c >= '2' && c <= '9') || c == '?' || c == '_';

    return rv;
}

/*
  This function checks if character can begin a suffix of octal number
*/
static inline bool test_char_octal_suffix(int c)
{
    bool rv;

    rv = test_char_lowercase(c) || test_char_uppercase(c);
    rv = rv || c == '8' || c == '9' || c == '?' || c == '_';

    return rv;
}

/*
  This fucntion checks if character can begin a suffix of decimal number
*/
static inline bool test_char_decimal_suffix(int c)
{
    bool rv;

    rv = test_char_lowercase(c) || test_char_uppercase(c);
    rv = rv || c == '?' || c == '_';

    return rv;
}

/*
  This function checks if character can begin a suffix of hexadecimal number
*/
static inline bool test_char_hexadecimal_suffix(int c)
{
    bool rv;

    rv = (c >= 'f' && c <= 'z') || (c >= 'F' && c <= 'Z');
    rv = rv || c == '?' || c == '_';

    return rv;
}

/*
  Functions test_char_${system}_suffix check if character can begin a suffix
  This function checks if character is a continuation of a suffix
  A set of characters that form continuation of a suffix is
  a union of characters that can form a number in given numeral system
  and characters that form an indentifier or in other words
  all identifier characters plus dot and comma
  It's the same for all supported numeral systems
*/
static inline bool test_char_suffix(int c)
{
    bool rv;

    rv = test_char_lowercase(c) || test_char_uppercase(c);
    rv = rv || c == '?' || c == '_';
    rv = rv || test_char_decimal(c) || c == ',' || c == '.';

    return rv;
}

/*
  Invalid sequence characters are these characters that preceeded by dot
  or comma make the number invalid and that themselves can form a number
  While g-z that follows a comma in a hexadecimal number does make the
  number invalid they can't form a number as hexadecimal characters
  are a-f A-F 0-9 , .
*/
static inline bool test_char_sequence(int c)
{
    return c == '.' || c == ',';
}

/*
  The name following comes from the fact that it is used by identifier states
  to check if a character can be part of an identifier
  The first character is checked with test_char_lowercase and
  test_char_uppercase and all the following characters with this function
*/
static inline bool test_char_following(int c)
{
    bool rv;

    rv = test_char_lowercase(c) || test_char_uppercase(c);
    rv = rv || test_char_decimal(c) || c == '?' || c == '_';

    return rv;
}
