// Any utility functions for the regex are stored here

#include <string.h> // For strlen


/**
 * Checks if the character is in the character array
 * Returns 1 on success, 0 on failure
 */
int ch_in_chs(char ch, char *chars) {
    int chars_len = strlen(chars);

    for (int i = 0; i < chars_len; i++) {
        if (ch == *(chars + i))
            return 1;
    }
    return 0;
}
