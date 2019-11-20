/**
 * Features currently implemented:
 * Matching literal characters
 * 
 * Metacharacters:
 * . (Any char) 
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strlen

#include "regex_utils.h"


#define MAX_BUFFER_SIZE 64

//static char *meta_characters = "[\\^$.|?*+()";


/***** Datatypes *****/
/**
 * Short_hands aren't included in StateType because they should get converted to something else
 * e.g \w get's changed to a character set matching [a-zA-Z0-9_]
 */
typedef enum {
    M_ANY_CH = 1, // .
} MetaChTypes;

typedef enum {
    // Special States
    S_START = 1,
    S_FINAL,

    // Normal States
    S_LITERAL_CH,
    S_META_CH,
    S_RANGE,
} StateType;

typedef union StateData_ {
    char ch; // for literal characters
    MetaChTypes meta; // Meta character types
} StateData;

typedef struct State_ {
    StateType type;

    union StateData_ data;

    struct State_ *next1;
    struct State_ *next2;
} State;


// Fragments of the state machine
typedef struct Fragment_ {
    struct State_ *start;

    State **l[MAX_BUFFER_SIZE];
    int n;
} Fragment;





/***** Function Prototypes *****/
Fragment pattern_to_fsm(char **pattern);

char *perform_regex(State *start, char *string);

/***** Utility functions *****/
State *create_state(StateType type, StateData data, State * const next1, State * const next2);
static inline void next_state(State **l, int *p, State **s, char **string);


char *regex(char *pattern, char *string) {
    // Echoing back for no real reason
#ifndef REGEX_DEBUG
    printf("Pattern : \"%s\"\n", pattern);
    printf("String  : \"%s\"\n", string);
#endif

    Fragment complete = pattern_to_fsm(&pattern);
    //char *return_str = perform_regex(start_state, string);
    char *return_str = "";

#ifndef REGEX_DEBUG
    if (*return_str == '\0')
        printf("Regex Failed\n");
    else
        printf("Returned string : %s\n", return_str);
#endif
    return return_str;
}


/**
 * Converts the pattern supplied to a Finite State Machine Fragment
 * Returns a single fragment
 */
Fragment pattern_to_fsm(char **pattern) {
    Fragment rtn;
    return rtn;
}

// Naviagtes the FSM and (should) returns the matched sub-string
char *perform_regex(State *start, char *string) {
    // We would do some storage of states that we can backtrack to
    State *s = start->next1;
    State *backtrack_list[MAX_BUFFER_SIZE];
    int btp = 0;
    if (start->next2) {
        backtrack_list[btp++] = start->next2;
    }

    printf("\n");

    while(1) {
        switch(s->type) {

            case S_META_CH:
                switch (s->data.meta) {
                    case M_ANY_CH:
                        // Any ch matches anything that isn't '\0'
                        if (*string != '\0') {
                            printf("State %p\tMeta character '.' (M_ANY_CH) matched with \"%c\"\n",
                                    (void *) s, *string);

                            next_state(backtrack_list, &btp, &s, &string);

                        } else {
                            printf("State %p\tMeta character '.' (M_ANY_CH) didn't match with \"%c\"\n",
                                    (void *) s, *string);
                            return ""; // Backtracking goes here
                        }
                        break;
                    default:
                        printf("Error in perform regex: Unknown MetaChType\n");
                        break;
                }
                break;

            case S_LITERAL_CH:
                if (s->data.ch == *string) {
                    printf("State %p\tLiteral character \"%c\" matched with \"%c\"\n",
                            (void *) s, s->data.ch, *string);

                    next_state(backtrack_list, &btp, &s, &string);

                } else {
                    // Backtrack
                    printf("State %p\tLiteral character \"%c\" didn't match with \"%c\"\n",
                            (void *) s, s->data.ch, *string);
                    return "";
                }
                break;

            case S_FINAL:
                return "Completed Matching successfully";

            default:
                printf("Error in perform regex: default hit on the switch statement\n");
                break;
        }
    }
    return "";
}


/***** Utility functions *****/
State *create_state(StateType type, StateData data, State * const next1, State * const next2) {
    State *a = malloc(sizeof(State));
    a->type  = type;
    a->data  = data;
    a->next1 = next1;
    a->next2 = next2;

    return a;
}

// Grouping together some common statements
static inline void next_state(State **l, int *p, State **s, char **string) {
    if ((*s)->next2) {
        l[*p++] = (*s)->next2;
    }

    *s = (*s)->next1;
    (*string)++;
}
