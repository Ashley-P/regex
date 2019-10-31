/**
 * Features currently implemented:
 * None
 */



#include <stdio.h>
#include <stdlib.h>



/***** Structs *****/
/**
 * Short_hands aren't included in StateType because they should get converted to something else
 * e.g \w get's changed to a character set matching [a-zA-Z0-9_]
 */
typedef enum {
    LITERAL_CH = 1,
    META_CH    = 2,
    RANGE      = 4, // For character sets (e.g [A-Z])
    FINAL      = 8, // Special State that ends the regex engine (Might not be needed actually)
} StateType;

typedef struct State_ {
    StateType type;
    void *a;

    struct State_ *next1;
    struct State_ *next2;
} State;


typedef enum {
    LITERAL_STR = 1, // Strings that can easily be converted to states (e.g abc)
    META_CH     = 2, // e.g . + ?
    RANGE       = 4, // For character sets (e.g [A-Z])
    SHORT_HAND  = 8, // e.g \w \b
} FragType;

typedef struct Frag_ {
    FragType type;
    char *str;
} Frag;



/***** Function Prototypes *****/
State *pattern_to_fsm(char *pattern);
Frag *pattern_fragmenter(char *pattern);





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
    


    State *start_state = malloc(sizeof(State));
    State *current_state = start_state;

    return start_state;
}
