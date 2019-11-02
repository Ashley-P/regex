/**
 * Features currently implemented:
 * None
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strlen

#include "regex_utils.h"


#define MAX_BUFFER_SIZE 64

static char *meta_characters = "[\\^$.|?*+()";


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
int collect_literal_string(Fragment *frag, char *pattern);





void regex(char *pattern, char *string) {
    // Echoing back for no real reason
#ifndef REGEX_DEBUG
    printf("Pattern : \"%s\"\n", pattern);
    printf("String  : \"%s\"\n", string);
#endif

    State *start_state = pattern_to_fsm(pattern);

#ifndef REGEX_DEBUG
    if (start_state == NULL) {
        //printf("Something went wrong\n");
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

#ifndef REGEX_DEBUG
    printf("frag_pos : %d\n", frags_pos);
#endif

#if 0
    State *start_state = malloc(sizeof(State));
    State *current_state = start_state;

    return start_state;

#endif
    return NULL;
}


// Fragments the pattern into manageable chunks to allow for easier creation of the FSM
Fragment pattern_fragmenter(char *pattern, int *str_pos) {
    Fragment frag;

    // The character at str_pos will tell us what to do
    switch (*(pattern + *str_pos)) {
        case '[': case '\\': case '^': case '$': case '.': case '|':
        case '?': case '*':  case '+': case '(': case ')':
            printf("Meta characters aren't handled yet, regex not completed");
            exit(0);
            break; // Is this necessary?

        default: // Every other character. The fragment type is a F_LITERAL_STRING
#ifndef REGEX_DEBUG
            printf("Literal character : %c\n", *(pattern + *str_pos));
#endif
            frag.type = F_LITERAL_STR;
            // Collect the rest of the string in the fragment
            *str_pos = collect_literal_string(&frag, (pattern + *str_pos));
            break;
    }

#ifndef REGEX_DEBUG
    printf("Frag type : %d\n", frag.type);
    printf("Frag str  : %s\n", frag.str);
#endif

    return frag;
}



// Collects as much as possible into a fragment of type F_LITERAL_STR
int collect_literal_string(Fragment *frag, char *pattern) {
    int a = 0;
    while (!ch_in_chs(*pattern, meta_characters) && *pattern != '\0') {
#ifndef REGEX_DEBUG
        printf("ch : %c\n", *pattern);
#endif
        frag->str[a++] = *pattern++;
    }

    return a;
}
