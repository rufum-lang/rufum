#include <stdbool.h>

static inline bool test_char_lowercase(int c)
{
    return c >= 'a' && c <= 'z';
}

static inline bool test_char_uppercase(int c)
{
    return c >= 'A' && c <= 'Z';
}

static inline bool test_char_binary(int c)
{
    return c == '0' || c == '1';
}

static inline bool test_char_octal(int c)
{
    return c >= '0' && c <= '7';
}

static inline bool test_char_decimal(int c)
{
    return c >= '0' && c <= '9';
}

static inline bool test_char_hexadecimal(int c)
{
    bool rv;

    rv = test_char_decimal(c);
    rv = rv || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');

    return rv;
}

static inline bool test_char_binary_suffix(int c)
{
    bool rv;

    rv = test_char_lowercase(c) || test_char_uppercase(c);
    rv = rv || (c >= '2' && c <= '9') || c == '?' || c == '_';
}

static inline bool test_char_octal_suffix(int c)
{
    bool rv;

    rv = test_char_lowercase(c) || test_char_uppercase(c);
    rv = rv || c == '8' || c == '9' || c == '?' || c == '_';

    return rv;
}

static inline bool test_char_decimal_suffix(int c)
{
    bool rv;

    rv = test_char_lowercase(c) || test_char_uppercase(c);
    rv = rv || c == '?' || c == '_';

    return rv;
}

static inline bool test_char_hexadecimal_suffix(int c)
{
    bool rv;

    rv = (c >= 'f' && c <= 'z') || (c >= 'F' && c <= 'Z');
    rv = rv || c == '?' || c == '_';

    return rv;
}

static inline bool test_char_sequence(int c)
{
    return c == '.' || c == ',';
}

static inline bool test_char_following(int c)
{
    bool rv;

    rv = test_char_lowercase(c) || test_char_uppercase(c);
    rv = rv || test_char_decimal(c) || c == '?' || c == '_';

    return rv;
}
