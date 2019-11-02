/**
 * Features currently implemented:
 * None
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strlen


#define MAX_BUFFER_SIZE 64


/***** Structs *****/
/**
 * Short_hands aren't included in StateType because they should get converted to something else
 * e.g \w get's changed to a character set matching [a-zA-Z0-9_]
 */
typedef enum {
    S_LITERAL_CH = 1,
    S_META_CH    = 2,
    S_RANGE      = 4, // For character sets (e.g [A-Z])
    S_FINAL      = 8, // Special State that ends the regex engine (Might not be needed actually)
} StateType;

typedef struct State_ {
    StateType type;
    void *a;

    struct State_ *next1;
    struct State_ *next2;
} State;


typedef enum {
    F_LITERAL_STR = 1, // Strings that can easily be converted to states (e.g abc)
    F_META_CH     = 2, // e.g . + ?
    F_RANGE       = 4, // For character sets (e.g [A-Z])
    F_SHORT_HAND  = 8, // e.g \w \b
} FragmentType;

typedef struct Fragment_ {
    FragmentType type;
    char str[MAX_BUFFER_SIZE];
} Fragment;



/***** Function Prototypes *****/
State *pattern_to_fsm(char *pattern);
Fragment pattern_fragmenter(char *pattern, int *str_pos);





void regex(char *pattern, char *string) {
    // Echoing back for no real reason
#ifndef REGEX_DEBUG
    printf("Pattern : \"%s\"\n", pattern);
    printf("String  : \"%s\"\n", string);
#endif

    State *start_state = pattern_to_fsm(pattern);

#ifndef REGEX_DEBUG
    if (start_state == NULL) {
        printf("Something went wrong\n");
        exit(0);
    } else {
        printf("%p\n", start_state);
    }
#endif
}


// Converts the pattern supplied to a Finite State Machine
State *pattern_to_fsm(char *pattern) {
    // First we break the pattern down into pattern fragments
    int pattern_len = strlen(pattern);
    int str_pos = 0;
    int frags_pos = 0;
    Fragment *frags = malloc(sizeof(Fragment) * MAX_BUFFER_SIZE);

    // Actually fragmenting the pattern
    while (str_pos < pattern_len) {
        *(frags + frags_pos++) = pattern_fragmenter(pattern, &str_pos);
    }

#if 0
    State *start_state = malloc(sizeof(State));
    State *current_state = start_state;

    return start_state;

#endif
    return NULL;
}


Fragment pattern_fragmenter(char *pattern, int *str_pos) {
    //Fragment *rtn_frag = malloc(sizeof(Fragment));
    printf("str_pos = %d\t", *str_pos);
    printf("char = %c\n", *(pattern + *str_pos));
    (*str_pos)++;
}
