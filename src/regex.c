#include <stdio.h>

typedef enum {
    LITERAL_CH = 1,
    META_CH    = 2,
    RANGE      = 4, // For character sets e.g [A-Z]
} StateType;

typedef struct State_ {
    StateType type;
    void *a;

    struct State_ *next1;
    struct State_ *next2;
} State;

void regex(char *pattern, char *string) {
    // Echoing back for no real reason
#ifndef REGEX_DEBUG
    printf("Pattern : \"%s\"\n", pattern);
    printf("String  : \"%s\"\n", string);
#endif
}
