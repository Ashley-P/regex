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


/***** Function Prototypes *****/
State *pattern_to_fsm(char *pattern);

char *perform_regex(State *start, char *string);


// regex specific utility functions
State *create_state(StateType type, StateData data, State * const next1, State * const next2);
void delete_states(State *s, State **list, int *lp);


char *regex(char *pattern, char *string) {
    // Echoing back for no real reason
#ifndef REGEX_DEBUG
    printf("Pattern : \"%s\"\n", pattern);
    printf("String  : \"%s\"\n", string);
#endif

    State *start_state = pattern_to_fsm(pattern);
    
#if 0
    int str_len = strlen(string);
    int sp = 0; // String pointer

    char *return_str = "";
    while (sp != str_len) {
#ifndef REGEX_DEBUG
        printf("\n\nRegex Iteration %d\n\n", sp);
#endif
        return_str = perform_regex(start_state, (string + sp));
        if (return_str[0] == '\0')
            sp++;
        else
            break;

    }


    // TODO: Correctly free memory at some point
    int lp = 0;
    State **delete_list = malloc(sizeof(State *) * MAX_BUFFER_SIZE);
    delete_states(start_state, delete_list, &lp);
    free(delete_list);
#endif
    char *return_str = "";
#ifndef REGEX_DEBUG
    printf("Returned string : %s", return_str);
#endif
    return return_str;
}


// Converts the pattern supplied to a Finite State Machine
State *pattern_to_fsm(char *pattern) {
    int pattern_len = strlen(pattern);
    int str_pos = 0;

    return NULL;
}

// Naviagtes the FSM and (should) returns the matched sub-string
char *perform_regex(State *start, char *string) {
    //int str_len = strlen(string) + 1; // We include the terminating character
    int sp = 0; // String Pointer
    //int sbp = 0; // Stack Backtrack Pointer

    //State **backtrack = malloc(sizeof(State *) * MAX_BUFFER_SIZE); // Keeping track of the backtracking positions
    // @TODO: Construct the string that gets returned
    State *s = start;

    while (1) {
#if 0 // Might not be needed since characters won't match with the terminating character
        if (sp > str_len) {
            printf("Regex Failed: End of string before end of FSM\n");
            return "";
        }
#endif

        switch(s->type) {
            case S_START:
                // This state exists to be a back track in case of alternations etc
                s = s->next1;
                break;
            case S_LITERAL_CH:
                if (s->data.ch == *(string + sp)) {

#ifndef REGEX_DEBUG
                    printf("State %p, Type %d, StateData \"%c\" matched with character \"%c\"\n",
                            (void *) s, s->type, s->data.ch, *(string + sp));
#endif
                    s = s->next1;
                    sp++;
                } else {
                    // @NOTE @TODO: Normally we would backtrack here
#ifndef REGEX_DEBUG
                    // @TODO: Come up with a way better debug message
                    printf("Regex Failed: pattern \"%c\" does not match string \"%c\"\n",
                            s->data.ch, *(string + sp));
#endif
                    return "";
                }
                break;
            case S_FINAL:
#ifndef REGEX_DEBUG
                return("Completed FSM");
#endif
                break;
            default:
#ifndef REGEX_DEBUG
                printf("Error in switch statement in \"perform_regex\" on line %d:\n", __LINE__);
                printf("Unknown/Unhandled type %d\n", start->type);
#endif
                break;
        }
    }

    return "";
}


/***** Utility functions *****/
State *create_state(StateType type, StateData data, State * const next1, State * const next2) {
    State *a = malloc(sizeof(State));
    a->type = type;
    a->data = data;
    a->next1 = next1;
    a->next2 = next2;

    return a;
}

/**
 * Frees the states by recursively going through them all and freeing if they are not on the list
 */
void delete_states(State *s, State **list, int *lp) {
    int a = 0;
    while (a < *lp) {
        if (s == *(list + a)) {
            return;
        }
        a++;
    }
    if (s->next1) delete_states(s->next1, list, lp);
    if (s->next2) delete_states(s->next1, list, lp);
    *(list + (*lp)++) = s;
    free(s);

#ifndef REGEX_DEBUG
    printf("State(s) freed : %d\n", *lp);
#endif

}
