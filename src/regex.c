#include <stdio.h>
#include <stdlib.h>



// Structs
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



// Function Prototypes
State *pattern_to_fsm(char *pattern);





void regex(char *pattern, char *string) {
    // Echoing back for no real reason
#ifndef REGEX_DEBUG
    printf("Pattern : \"%s\"\n", pattern);
    printf("String  : \"%s\"\n", string);
#endif

    State *start_state = pattern_to_fsm(pattern);
    if (start_state == NULL) {
        printf("Something went wrong\n");
        exit(0);
    }

}

// Converts the pattern supplied to a Finite State Machine
State *pattern_to_fsm(char *pattern) {

    return NULL;
}
