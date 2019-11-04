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
    S_START      = 8, // Special State, might not be needed
    S_FINAL      = 16, // Special State that ends the regex engine (Might not be needed actually)
} StateType;

typedef union StateData_ {
    char ch; // for literal characters
} StateData;

typedef struct State_ {
    StateType type;

    union StateData_ data;

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

State **fragments_to_state(Fragment *frags, int fp);
State *literal_str_to_states(Fragment frag);

State *link_state_chunks(State **states);


// regex specific utility functions
State *create_state(StateType type, StateData data, State * const next1, State * const next2);


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
        printf("%p\n", (void *) start_state);
    }
#endif

    // TODO: Correctly free memory at some point

#ifndef REGEX_DEBUG
    printf("Regex engine reached the end!");
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

    State **state_chunks = fragments_to_state(frags, frags_pos);
    State *start_state = link_state_chunks(state_chunks);


    return start_state;
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
    frag->str[a] = '\0';

    return a;
}


/**
 * Returns a list of state chunks
 */
State **fragments_to_state(Fragment *frags, int fp) {
    int a = 0; // frags pointer but for the local scope
    int b = 0; // states pointer

    // Keep a list of the states so we can link them together later
    State **states = malloc(sizeof(State) * MAX_BUFFER_SIZE);

    while (a < fp) {
        switch ((frags + a)->type) {
            case F_LITERAL_STR:
                *(states + b++) = literal_str_to_states(*(frags + a));
                break;
            case F_META_CH: // We shouldn't hit these yet but to make sure
            case F_RANGE:
            case F_SHORT_HAND:
            default:
                printf("Case not handled in \"fragments_to_state\", regex not completed");
                exit(0);
                break;
        }
        a++;
    }

    // Create the final state here and put it in the array so we can size it up properly
    StateData final_data = {.ch = '\0'};
    *(states + b++) = create_state(S_FINAL, final_data, NULL, NULL);

    return states;
}

// Converts a fragment of type F_LITERAL_STR to some states
State *literal_str_to_states(Fragment frag) {
    int a = 0;
    int frag_len = strlen(frag.str);

    // frag_len is atleast 1 so we do the first state manually and set off the loop
    // Since it's just a string, next1 of each state links up and next2 is left as NULL

    // A reusable union
    StateData data = {.ch = frag.str[a++]};
    State *start = create_state(S_LITERAL_CH, data, NULL, NULL);


    State *prev = start;
    while (a < frag_len) {
#ifndef REGEX_DEBUG
        printf("%d\n", a);
        printf("State type : %d\n", prev->type);
        printf("State ch   : %c\n", prev->data.ch);
#endif

        data.ch = frag.str[a++];
        prev->next1 = create_state(S_LITERAL_CH, data, NULL, NULL);
        prev = prev->next1;
    }

#ifndef REGEX_DEBUG
    // Print out the last state too
    printf("%d\n", a);
    printf("State type : %d\n", prev->type);
    printf("State ch   : %c\n", prev->data.ch);
#endif

    return start;
}


// Links the chunks together into the Finite State Machine
State *link_state_chunks(State **states) {
    int sc_len = 0; //state chunk length
    while ((*(states + sc_len))->type != S_FINAL) {
        sc_len++;
    }
    // Here we would link the states together properly
    printf("%d State chunk(s) need linking\n", sc_len);

    // Creating the start state
    StateData start_data = {.ch = '\0'};
    State *start_state = create_state(S_START, start_data, *(states), NULL);

    /**
     * @NOTE: Some weird fiddling would have to happen if there was an alternation
     * But right now we can link the start state with the first state in the list
     * Lots of other weird fiddling with states with the other meta characters too
     */

    return start_state;
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
