/**
 * Features currently implemented:
 * Matching literal characters
 * 
 * Metacharacters:
 * . (Any char) 
 */



#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strlen

#include "regex.h"



/***** Defines *****/
#define MAX_STACK_SIZE 64
#define MAX_STRING_SIZE 256


/***** Datatypes *****/
/**
 * Short_hands aren't included in StateType because they should get converted to something else
 * e.g \w get's changed to a character set matching [a-zA-Z0-9_]
 */
typedef enum {
    M_ANY_CH = 1, // .
} MetaChType;

typedef enum {
    // Special States
    S_FINAL = 1,
    S_NODE,

    // Normal States
    S_LITERAL_CH,
    S_META_CH,
    S_CCLASS,
} StateType;

typedef union StateData_ {
    char ch; // for literal characters
    MetaChType meta; // Meta character types
    char *cclass;
} StateData;

typedef struct State_ {
    StateType type;

    union StateData_ data;

    struct State_ *next1;
    struct State_ *next2;
} State;


typedef struct StateList_ {
    struct State_ **l[MAX_STACK_SIZE];
    int n;
} StateList;

// Fragments of the state machine
typedef struct Fragment_ {
    struct State_ *start;
    struct StateList_ *list;
} Fragment;

typedef struct BacktrackData_ {
    char *string;
    char *sp;
    State *s;
} BacktrackData;


/***** Constants *****/
static int suppress_logging = 0;





/***** Function Prototypes *****/
char *regex(char *pattern, char *string, unsigned int options);
char *pre_parse_pattern(char *pattern);
Fragment parse_pattern(char *pattern);
char *perform_regex(State *start, char *string);

int check_pattern_correctness(char *pattern);
int state_altering_check(char *p);
int character_class_check(char *p);



/***** Utility functions *****/
Fragment *link_fragments(Fragment *fp, char *sp);
State *create_state(StateType type, StateData data, State * const next1, State * const next2);
Fragment create_fragment(State *s, StateList *l);
void point_state_list(StateList *l, State *a);
StateList *create_state_list(State **first);
StateList *append_lists(StateList *a, StateList *b);
char *create_character_class(char *sp, StateData *data);

BacktrackData create_backtrack_data(char *string, char *sp, State *s);

char *state_type_to_string(StateType type);
char *meta_ch_type_to_string(MetaChType type);

// String stuff
int match_ch_str(char ch, char *str);
static inline char peek_ch(char *str);

void pattern_error(char *p, unsigned int pos, unsigned int range, char *msg, ...);
void regex_log(char *msg, ...);



char *regex(char *pattern, char *string, unsigned int options) {
    // Handle options - should be moved to it's own function
    if (options & REGEX_SUPPRESS_LOGGING) suppress_logging = 1;

    // Echoing back for no real reason
    regex_log("Pattern : \"%s\"\n", pattern);
    regex_log("String  : \"%s\"\n", string);

    // We'd do some checking here maybe
    if (check_pattern_correctness(pattern))
        return "";

    char *new_pattern = pre_parse_pattern(pattern);
    Fragment fsm = parse_pattern(new_pattern);
    State *start = fsm.start;

    // Adding the final state to the final fsm
    StateData d = {.ch = '\0'};
    State *final = create_state(S_FINAL, d, NULL, NULL);
    point_state_list(fsm.list, final);
    regex_log("\nFinal State %p, Node State\n", (void *) final);

    // Do the regex at each point of the string
    char *return_str = "";
    for (int i = 0; *(string + i) != '\0'; i++) {
        regex_log("\n\nRegex Iteration %d\n", i + 1);
        return_str = perform_regex(start, string + i);
        if (*return_str != '\0')
            break;
        else
            regex_log("\nIteration %d failed\n\n", i + 1);
    }

    if (*return_str == '\0')
        regex_log("Regex Failed\n");
    else
        regex_log("Returned string : %s\n\n", return_str);
    return return_str;
}


// Changing up the pattern slightly so that the parsing works
char *pre_parse_pattern(char *pattern) {
    return pattern;
}

Fragment parse_pattern(char *pattern) {
    regex_log("\n----- Parsing pattern -----\n");
    Fragment stack[MAX_STACK_SIZE];
    Fragment *fp = &stack[0]; // fragments pointer
    Fragment a, b;

    char *sp = pattern;; // string pointer
    
    StateData data;
    data.ch = '\0';
    State *s = create_state(S_NODE, data, NULL, NULL);
    // a start start for the entire fragment stack

    *fp++ = create_fragment(s, create_state_list(&s->next1));

    while (*sp != '\0') {
        switch (*sp) {
            case '[':
                sp = create_character_class(++sp, &data);
                s = create_state(S_CCLASS, data, NULL, NULL);
                *fp++ = create_fragment(s, create_state_list(&s->next1));

                // check if we should link this fragment with the one before it
                fp = link_fragments(fp, sp);
                regex_log("State %p, Type = %s, range = %s\n",
                        (void *) s, state_type_to_string(s->type), s->data.cclass);
                sp++;
                break;

            case '?':
                a = *--fp;
                data.ch = '\0';

                s = (peek_ch(sp) == '?') ? create_state(S_NODE, data, NULL, a.start)
                                         : create_state(S_NODE, data, a.start, NULL);

                //point_state_list(a.list, s);

                *fp++ = (peek_ch(sp) == '?')
                          ? create_fragment(s, append_lists(a.list, create_state_list(&s->next1)))
                          : create_fragment(s, append_lists(a.list, create_state_list(&s->next2)));

                sp += (peek_ch(sp) == '?') ? 1 : 0;
                fp = link_fragments(fp, sp);
                sp++;
                regex_log("State %p, Node State\n", (void *) s);
                break;

            case '*':
                a = *--fp;
                data.ch = '\0';

                // Since there are minor differences between the lazy and greedy versions
                // We can just use the ternary operator and save some redundancy
                // @NOTE : Don't chain ternary operators together
                s = (peek_ch(sp) == '?') ? create_state(S_NODE, data, NULL, a.start)
                                         : create_state(S_NODE, data, a.start, NULL);

                point_state_list(a.list, s);

                *fp++ = (peek_ch(sp) == '?') ? create_fragment(s, create_state_list(&s->next1))
                                             : create_fragment(s, create_state_list(&s->next2));


                sp += (peek_ch(sp) == '?') ? 1 : 0;
                fp = link_fragments(fp, sp);
                sp++;
                regex_log("State %p, Node State\n", (void *) s);

                break;
            case '+':
                a = *--fp;
                data.ch = '\0';

                s = (peek_ch(sp) == '?') ? create_state(S_NODE, data, NULL, a.start)
                                         : create_state(S_NODE, data, a.start, NULL);

                point_state_list(a.list, s);

                *fp++ = (peek_ch(sp) == '?') ? create_fragment(a.start, create_state_list(&s->next1))
                                             : create_fragment(a.start, create_state_list(&s->next2));

                sp += (peek_ch(sp) == '?') ? 1 : 0;
                fp = link_fragments(fp, sp);
                sp++;
                regex_log("State %p, Node State\n", (void *) s);

                break;

            case '.':
                data.meta = M_ANY_CH;
                s = create_state(S_META_CH, data, NULL, NULL);
                *fp++ = create_fragment(s, create_state_list(&s->next1));

                fp = link_fragments(fp, sp);
                regex_log("State %p, Type = %s, ch = %c\n",
                        (void *) s, state_type_to_string(s->type), s->data.ch);
                sp++;
                break;

            default: // Normal characters
                data.ch = *sp;
                s = create_state(S_LITERAL_CH, data, NULL, NULL);
                *fp++ = create_fragment(s, create_state_list(&s->next1));

                // check if we should link this fragment with the one before it
                fp = link_fragments(fp, sp);
                regex_log("State %p, Type = %s, ch = %c\n",
                        (void *) s, state_type_to_string(s->type), s->data.ch);
                sp++;
                break;
        }
    }


    return stack[0];
}

// Naviagtes the FSM and (should) returns the matched sub-string
char *perform_regex(State *start, char *string) {
    regex_log("\n----- Navigating Finite State Machine -----\n");

    BacktrackData backtrack_stack[MAX_STACK_SIZE];
    BacktrackData *btp = backtrack_stack;
    BacktrackData b;
    int do_backtrack = 0;

    // State *s = start->next1; // This is the state we are checking
    State *s = start;
    char *rtn_str = malloc(sizeof(char) * MAX_STRING_SIZE);
    char *sp = rtn_str;
#if 0
    if (start->next2)
        *btp++ = create_backtrack_data(string, sp, start->next2);
#endif


    while (1) {
        regex_log("State %p, ", (void *) s);

        switch (s->type) {
            case S_CCLASS:
                // If we get a match
                if (match_ch_str(*string, s->data.cclass)) {
                    regex_log("Character class \"%s\" matched with character \"%c\" in string \n",
                           s->data.cclass, *string);

                    *sp++ = *string++;
                    if (s->next2)
                        *btp++ = create_backtrack_data(string, sp, s->next2);

                    s = s->next1;
                } else {
                    regex_log("Character class \"%s\" did not match with character \"%c\" in string \n",
                           s->data.cclass, *string);
                    do_backtrack = 1;
                }
                break;
            case S_META_CH:

                switch (s->data.meta) {
                    // Matches anything but the null character
                    case M_ANY_CH:
                        if (*string != '\0') {
                            regex_log("Meta Character \".\" matched character \"%c\" in string \n", *string);

                            *sp++ = *string++;
                            if (s->next2)
                                *btp++ = create_backtrack_data(string, sp, s->next2);

                            s = s->next1;
                        } else {
                            regex_log("Meta Character \".\" did not match character \"%c\" in string \n", *string);
                            do_backtrack = 1;
                        }
                        break;
                    default:
                        regex_log("Shouldn't hit this\n");
                        return "";
                }

                break;

            case S_LITERAL_CH:
                if (s->data.ch == *string) {
                    regex_log("Literal character \"%c\" matched character \"%c\" in string\n",
                            s->data.ch, *string);

                    *sp++ = *string++;
                    if (s->next2)
                        *btp++ = create_backtrack_data(string, sp, s->next2);
                    s = s->next1;
                } else {
                    // Backtracking would happen here
                    regex_log("Literal character \"%c\" did not match character \"%c\" in string\n",
                            s->data.ch, *string);
                    do_backtrack = 1;
                }
                break;

            // Specials
            case S_FINAL:
                regex_log("Match completed!\n");
                *sp++ = '\0';
                return rtn_str;
            
            case S_NODE:
                regex_log("Node State, next1 = %p, next2 = %p\n", (void *) s->next1, (void *) s->next2);
                if (s->next2)
                    *btp++ = create_backtrack_data(string, sp, s->next2);
                s = s->next1;
                break;
            default:
                regex_log("Shouldn't hit this\n");
                return "";
        } // Switch

        if (do_backtrack) {
            regex_log("\nAttempting to backtrack\n");

            if (backtrack_stack == btp) {
                regex_log("Stack empty, unable to backtrack\n");
                return "";
            } else {
                b = *--btp;
                string = b.string;
                sp = b.sp;
                s = b.s;
                regex_log("Backtrack complete: Starting at state %p\n\n", (void *) s);
                do_backtrack = 0;
            }
        }
    } // While
}


// Returns 0 if the pattern is correct
int check_pattern_correctness(char *pattern) {
    int a = 0;
    a += state_altering_check(pattern);
    a += character_class_check(pattern);
    return a;
}

/**
 * State altering characters can't be next to each other or at the start of the pattern
 * e.g "ab?+" "+ab"
 */
int state_altering_check(char *p) {
    // Checking the beginning of the pattern

    switch (*p) {
        case '*': case '+': case '?':
            pattern_error(p, 0, 0, "Meta character \"%c\" is not allowed at the start of the pattern\n", *p);
            return 1;
        default:
            break;
    }

    // Used to prevent erroneous errors about incorrect character usage inside character classes
    int inside_cclass = 0;
    char *c = p - 1;

    while (*++c != '\0') {
        if (*c == '[')
            inside_cclass = 1;
        else if (*c == '[')
            inside_cclass = 0;

        // Invalid setup e.g "?+"
        if ((*c == '*' || *c == '+' || *c == '?') && (*(c + 1) == '*' || *(c + 1) == '+')) {

            // Ignore if the previous character is esacped or we are in a character class 
            if (*(c - 1) != '\\' && inside_cclass != 1) {
                pattern_error(p, (int) ((c + 1) - p), 0,
                        "Meta character \"%c\" is not allowed after meta character \"%c\"\n",
                        *(c + 1), *c);
                return 1;

            }
        }
    }

    return 0;
}

int character_class_check(char *p) {
    // First we make sure that there is a closing square brace in the pattern
    // @FIXME: a pattern like [A-Z-a] will be parsed incorrectly
    char *c = p - 1;
    int inside_cclass = 0;
    char *sb;

    while (*++c != '\0') {
        if (inside_cclass == 1) {
            if (*c == ']')
                inside_cclass = 0;

            // We can check hyphens in here too
            if (*c == '-' && *(c - 1) != '\\') { // Checking if it's escaped
                if (*(c - 2) != '\\' && match_ch_str(*(c + 1), "]\\") == 0) { // Checking escapes
                    if (*(c + 1) < *(c - 1)) {
                        // should be range pattern error
                        pattern_error(p, (int) (c - p) - 1, 2,
                                "Character class error: Start of range is less than end of range");
                        return 1;
                    }
                }
                            
            }

        } else if (inside_cclass == 0) {
            if (*c == '[') {
                inside_cclass = 1;
                sb = c;
            } else if (*c == ']' && *(c - 1) != '\\') {
                // Brace has to be escaped
                pattern_error(p, (int) (c - p), 0, "Unmatched Brace in pattern");
                return 1;
            }
        }
    }

    if (inside_cclass == 1) {
        pattern_error(p, (int) (sb - p), 0, "Unmatched Brace in pattern");
        return 1;
    }

    return 0;
    
}


/***** Utility functions *****/
Fragment *link_fragments(Fragment *fp, char *sp) {
    Fragment a, b;

    if (match_ch_str(peek_ch(sp), "?+*") == 0) {
        b = *--fp;
        a = *--fp;
        point_state_list(a.list, b.start);
        *fp++ = create_fragment(a.start, b.list);
    }

    return fp;
}

State *create_state(StateType type, StateData data, State * const next1, State * const next2) {
    State *a = malloc(sizeof(State));
    a->type  = type;
    a->data  = data;
    a->next1 = next1;
    a->next2 = next2;

    return a;
}

Fragment create_fragment(State *s, StateList *l) {
    Fragment rtn;
    rtn.start = s;
    rtn.list = l;
    return rtn;
}

// The pointers from the first fragment are pointed the the start of the next one
void point_state_list(StateList *l, State *a) {
    for (int i = 0; i < l->n; i++) {
        *(l->l)[i] = a;
    }
}

StateList *create_state_list(State **first) {
    StateList *l = malloc(sizeof(StateList));
    l->n = 0;
    l->l[l->n++] = first;

    return l;
}

StateList *append_lists(StateList *a, StateList *b) {
    StateList *rtn = malloc(sizeof(StateList));
    rtn->n = 0;

    for (int i = 0; i < a->n; i++)
        rtn->l[i] = a->l[i];

    for (int i = 0; i < b->n; i++)
        rtn->l[i + a->n] = b->l[i];

    rtn->n = a->n + b->n;

    free(a);
    free(b);
    return rtn;
}


char *create_character_class(char *sp, StateData *data) {
    data->cclass = malloc(sizeof(char) * MAX_STRING_SIZE);
    char *cp = &data->cclass[0];
    // @TODO: We don't look for backwards slashes right now

    while (*sp != ']') {
        if (*sp == '-') {
            if (*(sp - 1) != '\\' && *(sp - 1) != '[' && peek_ch(sp) != '\\' && peek_ch(sp) != ']') {
                // We've already collected the first character
                for (int i = *(sp - 1) + 1; i <= peek_ch(sp); i++)
                    *cp++ = i;

            }
            sp += 2;
        } else // collect as normal
        *cp++ = *sp++; 
            
    }

    *cp++ = '\0';

    return sp;
}



BacktrackData create_backtrack_data(char *string, char *sp, State *s) {
    BacktrackData rtn;
    rtn.string = string;
    rtn.sp = sp;
    rtn.s = s;

    return rtn;
}

char *state_type_to_string(StateType type) {
    switch (type) {
        case S_FINAL: return "S_FINAL";
        case S_NODE: return "S_NODE";
        case S_LITERAL_CH: return "S_LITERAL_CH";
        case S_META_CH: return "S_META_CH";
        case S_CCLASS: return "S_CCLASS";
        default: return "Unhandled case in state_type_to_string";
    }
}

char *meta_ch_type_to_string(MetaChType type) {
    switch (type) {
        case M_ANY_CH: return "M_ANY_CH";
        default: return "Unhandled case in meta_ch_type_to_string";
    }
}

// returns 1 in the character is in the string
int match_ch_str(char ch, char *str) {
    int str_len = strlen(str);

    for (int i = 0; i < str_len; i++) {
        if (ch == *(str + i))
            return 1;
    }
    return 0;
}

static inline char peek_ch(char *str) {
    return *(str+1);
}

// Allows nice printing showing where the error is in the pattern
void pattern_error(char *p, unsigned int pos, unsigned int range, char *msg, ...) {
    static char error_msg[] = "Error in pattern -> ";
    printf("%s%s\n", error_msg, p);

    // Constructing the string that points to the error in the pattern
    char str[MAX_STRING_SIZE];
    char *sp = str;
    for (unsigned int i = 0; i < strlen(error_msg) + pos; i++) {
        *sp++ = ' ';
    }
    *sp++ = '^';
    for (unsigned int i = 0; i < range; i++)
        *sp++ = '~';
    *sp++ = '\n';
    *sp++ = '\0';
    printf(str);

    // Printing the message
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
}

// Let's us easily suppress printing to the screen probably temporary
void regex_log(char *msg, ...) {
    if (suppress_logging) return;

    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
}
