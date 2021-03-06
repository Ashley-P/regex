/**
 * Features currently implemented:
 * Matching literal characters
 * 
 * Metacharacters:
 * - '.'  (Any character except '\0')
 * - '+'  Greedy quantifier (1 or more)
 * - '+?' Lazy quantifier (1 or more)
 * - '?'  Greedy quantifier (0 or 1)
 * - '??' Lazy quantifier (0 or 1)
 * - '\*' Greedy quantifier (0 or more)
 * - '\*?' Lazy quantifier (0 or more)
 * - '|' Alternation
 * 
 * Other stuff:
 * - '[a-z]' Character classes including ranges
 * - '()' Capturing groups (No backreferencing though)
 */



#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strlen

#include "regex.h"



/***** Defines *****/
#define MAX_STACK_SIZE     64
#define MAX_STRING_SIZE    256
#define MAX_CAPTURE_GROUPS 100 // It's actually 99 but it's easier than putting + 1 everywhere
#define MAX_REGEX_MALLOC   10000

#define EXACT_QUANTIFIER     -1
#define OPEN_ENDED_QUANTIFIER -2


/***** Datatypes *****/
/**
 * Short_hands aren't included in StateType because they should get converted to something else
 * e.g \w get's changed to a character set matching [a-zA-Z0-9_]
 */
typedef enum {
    M_ANY_CH = 1, // .
} MetaChType;

typedef struct AQData_ {
    unsigned int max;
    unsigned int min;
    unsigned int visited;
             int lazy;
} AQData;

typedef enum {
    // Special States
    S_FINAL = 1,
    S_NODE,
    S_CG_NODE, // Store capture group data
    S_AQ_NODE,

    // Normal States
    S_LITERAL_CH,
    S_META_CH,
    S_CCLASS,
    S_REVERSE_CCLASS,
    S_BACK_REFERENCE,
} StateType;

typedef union StateData_ {
    unsigned char ch; // for literal characters
    MetaChType meta; // Meta character types
    char *cclass;
    char cg; // capture group number - negative number means we are leaving the group
    struct AQData_ aq;
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

typedef struct CaptureGroupData_ {
    int icg[MAX_CAPTURE_GROUPS];
    int str_ptrs[MAX_CAPTURE_GROUPS];
    char cgs[MAX_CAPTURE_GROUPS][MAX_STRING_SIZE];
} CaptureGroupData;

typedef struct BacktrackData_ {
    char *string;
    char *sp;
    State *s;

    // We also gotta keep track of where the capturing groups are upto
    struct CaptureGroupData_ *cgd;
} BacktrackData;


typedef struct Options_ {
    unsigned int suppress_logging : 1;
    unsigned int start_of_string  : 1; // When the '^' is used at the start of the string
} Options;

/***** Constants *****/
static Options options;

// used in parse_pattern to keep track of capturing groups. Only used for printing
static int capturing_group = 0;

// ezpz way to free stuff
static void **regex_free;
static void **rfp; // regex_free pointer
static Fragment err_fragment = {NULL, NULL};




/***** Function Prototypes *****/
char *regex(char *pattern, char *string, unsigned int opts);
static char *pre_parse_pattern(char *pattern);
static Fragment parse_pattern(char **pattern);
static char *perform_regex(State *start, char *string);
static void capture_group_str_grab(CaptureGroupData *cgd, char ch);

static int check_pattern_correctness(char *pattern);
static int state_altering_check(char *p);
static int character_class_check(char *p);



/***** Utility functions *****/
static Fragment *link_fragments(Fragment *fp, char *sp);
static State *create_state(StateType type, StateData data, State * const next1, State * const next2);
static Fragment create_fragment(State *s, StateList *l);
static void point_state_list(StateList *l, State *a);
static StateList *create_state_list(State **first);
static StateList *append_lists(StateList *a, StateList *b);

static char *create_character_class(char *sp, StateData *data);
static State *parse_escapes(char **p);

static int get_arbitrary_quantifier(char **p, int *a, int *b);

static BacktrackData create_backtrack_data(char *string, char *sp, State *s, CaptureGroupData *cgd);

static char *state_type_to_string(StateType type);
//static char *meta_ch_type_to_string(MetaChType type);

static void handle_options(unsigned int opts);


// String stuff
static int match_ch_str(char ch, char *str);
static inline char peek_ch(char *str);
static inline char reverse_peek_ch(char *str);

static void pattern_error(char *p, unsigned int pos, unsigned int range, char *msg, ...);
static void regex_log(char *msg, ...);



char *regex(char *pattern, char *string, unsigned int opts) {
    // Handle options - should be moved to it's own function
    handle_options(opts);
    regex_free = malloc(sizeof(void *) * MAX_REGEX_MALLOC);
    rfp = regex_free;

    // Echoing back for no real reason
    regex_log("Pattern : \"%s\"\n", pattern);
    regex_log("String  : \"%s\"\n", string);

    // We'd do some checking here maybe
    if (check_pattern_correctness(pattern))
        return "";

    char *new_pattern = pre_parse_pattern(pattern);
    Fragment fsm = parse_pattern(&new_pattern);
    // If we get back an error fragment then something's gone wrong
    if (fsm.start == NULL) {
        regex_log("Aborting regex\n");
        return "";
    }

    State *start = fsm.start;

    // Adding the final state to the final fsm
    StateData d = {.ch = '\0'};
    State *final = create_state(S_FINAL, d, NULL, NULL);
    point_state_list(fsm.list, final);
    regex_log("\nFinal State %p, Node State\n", (void *) final);

    char *return_str = "";

    if (options.start_of_string) {
        regex_log("\n\nStart of string only\n");
        regex_log("Regex Iteration 1\n");
        return_str = perform_regex(start, string);

    } else {
        // Do the regex at each point of the string
        for (int i = 0; *(string + i) != '\0'; i++) {
            regex_log("\n\nRegex Iteration %d\n", i + 1);
            return_str = perform_regex(start, string + i);
            if (*return_str != '\0')
                break;
            else
                regex_log("\nIteration %d failed\n\n", i + 1);
        }
    }

    if (*return_str == '\0')
        regex_log("Regex Failed\n");
    else
        regex_log("Returned string : %s\n\n", return_str);

    // Cleanup
    capturing_group = 0;

    while (rfp != regex_free) {
        free(*--rfp);
    }
    free(regex_free);

    return return_str;
}


// Changing up the pattern slightly so that the parsing works
static char *pre_parse_pattern(char *pattern) {
    return pattern;
}

static Fragment parse_pattern(char **pattern) {
    regex_log("\n----- Parsing pattern -----\n");
    //Fragment *stack = malloc(sizeof(Fragment) * MAX_STACK_SIZE);
    Fragment stack[MAX_STACK_SIZE];

    // For debugging
#if 1
    for (int i = 0; i < MAX_STACK_SIZE; i++)
        stack[i] = create_fragment(NULL, NULL);
#endif
    
    Fragment *fp = &stack[0]; // fragments pointer
    Fragment volatile a, b; // These get optimized out and it breaks alternation
    int cap_group_tmp; // For use when we get '('

    // Creating the first state
    // If we are in a capturing group, we change the data
    StateData data;
    data.cg = capturing_group;
    State *s;
    if (capturing_group != 0) {
        s = create_state(S_CG_NODE, data, NULL, NULL);
        regex_log("Start of parse_pattern: State %p, Capture Group Node State\n", (void *) s);
    } else {
        s = create_state(S_NODE, data, NULL, NULL);
        regex_log("Start of parse_pattern: State %p, Node State\n", (void *) s);
    }
    // a start for the entire fragment stack

    *fp++ = create_fragment(s, create_state_list(&s->next1));

    while (*(*pattern) != '\0') {
        switch (*(*pattern)) {
            case '{': // Arbitrary quantifiers
                regex_log("Arbitrary quantifier\n");
                data.aq.visited = UINT_MAX; // Overflows back to 0 on first visit
                int aq1, aq2;

                if (get_arbitrary_quantifier(pattern, &aq1, &aq2)) {


                    if (aq1 > aq2 && aq2 > 0) {
                        regex_log("Arbitrary quantifier minimum \"%d\" is greater than the maximum \"%d\"\n",
                                aq1, aq2);
                        return err_fragment;
                    }

                    if (peek_ch(*pattern) == '?') {
                        (*pattern)++; // Moving onto the question mark
                        data.aq.lazy = 1;
                    } else {
                        data.aq.lazy = 0;
                    }

                    if (aq1 == 0 && aq2 == -1) { // edge case {0}
                        regex_log("Edge case {0} handled\n");
                        (*pattern)++;
                        fp--;

                    } else if (aq1 == 0) { // exact/open-ended/arbitrary quantifier that start with zero 
                        // Realistically won't hit UINT_MAX
                        //aq2 == OPEN_ENDED_QUANTIFIER ? data.aq.max = UINT_MAX : data.aq.max = aq2;
                        data.aq.min = aq1;

                    } else { // exact/open-ended/arbitrary quantifier that start non zero 
                        data.aq.min = aq1;
                        data.aq.max = (aq2 == EXACT_QUANTIFIER) ? (unsigned int) aq1
                               : (aq2 == OPEN_ENDED_QUANTIFIER) ? UINT_MAX : (unsigned int) aq2;

                        // Works the same as case '+'
                        a = *--fp;
                        s = data.aq.lazy == 0 ? create_state(S_AQ_NODE, data, a.start, NULL)
                                              : create_state(S_AQ_NODE, data, NULL, a.start);
                        point_state_list(a.list, s);
                        *fp++ = data.aq.lazy == 0 ? create_fragment(s, create_state_list(&s->next2))
                                                  : create_fragment(s, create_state_list(&s->next1));

                        fp = link_fragments(fp, (*pattern));
                        (*pattern)++;
                        regex_log("Arbitray Quantifier: State %p, AQ Node State aq max %u, aq min %u",
                                (void *) s, data.aq.max, data.aq.min);
                        regex_log(", lazy = %s\n", data.aq.lazy == 1 ? "Yes" : "No");
                    }

                } else {
                    regex_log("Badly formatted arbitrary quantifier, parsing as literal characters\n");
                    goto def;
                }
                break;

            case '\\': // Escaped characters
                s = parse_escapes(pattern);
                *fp++ = create_fragment(s, create_state_list(&s->next1));

                // check if we should link this fragment with the one before it
                fp = link_fragments(fp, (*pattern));
                (*pattern)++;
                break;

            case '^':
                options.start_of_string = 1;
                (*pattern)++;
                break;

            case '(':
                regex_log("\nCapturing group %d\n", ++capturing_group);
                cap_group_tmp = capturing_group;
                (*pattern)++; // Moving into the paren

                a = parse_pattern(pattern); // Collect everything inside the parentheses
                data.cg = 0 - cap_group_tmp; // negating the number to show we are leaving the group
                s = create_state(S_CG_NODE, data, NULL, NULL);
                point_state_list(a.list, s);
                *fp++ = create_fragment(a.start, create_state_list(&s->next1));
                
                fp = link_fragments(fp, (*pattern)); // **pattern is ')' so we can check the next char

                (*pattern)++; // Move past the ')'
                regex_log("\nEnd of Capturing group %d\n\n", capturing_group);
                break;

            case ')':
                return *stack;

            case '[':
                if (*((*pattern) + 1) == '^') {
                    (*pattern) += 2;
                    (*pattern) = create_character_class(*pattern, &data);
                    s = create_state(S_REVERSE_CCLASS, data, NULL, NULL);
                } else {
                    (*pattern) = create_character_class(++(*pattern), &data);
                    s = create_state(S_CCLASS, data, NULL, NULL);
                }
                *fp++ = create_fragment(s, create_state_list(&s->next1));

                // check if we should link this fragment with the one before it
                fp = link_fragments(fp, (*pattern));
                regex_log("State %p, Type = %s, range = %s\n",
                        (void *) s, state_type_to_string(s->type), s->data.cclass);
                (*pattern)++;
                break;

            case '|':
                a = *--fp;
                (*pattern)++; // moving past '|'
                regex_log("\nAlternating\n");
                b = parse_pattern(pattern);

                data.ch = '\0';
                s = create_state(S_NODE, data, a.start, b.start);
                *fp++ = create_fragment(s, append_lists(a.list, b.list));
                fp = link_fragments(fp, (*pattern) - 1);
                regex_log("Alternation: State %p, Node State\n", (void *) s);
                break;

            case '?':
                a = *--fp;
                data.ch = '\0';

                s = (peek_ch((*pattern)) == '?') ? create_state(S_NODE, data, NULL, a.start)
                                         : create_state(S_NODE, data, a.start, NULL);

                //point_state_list(a.list, s);

                *fp++ = (peek_ch((*pattern)) == '?')
                          ? create_fragment(s, append_lists(a.list, create_state_list(&s->next1)))
                          : create_fragment(s, append_lists(a.list, create_state_list(&s->next2)));

                (*pattern) += (peek_ch((*pattern)) == '?') ? 1 : 0;
                fp = link_fragments(fp, (*pattern));
                (*pattern)++;
                regex_log("0 Or 1: State %p, Node State\n", (void *) s);
                break;

            case '*':
                a = *--fp;
                data.ch = '\0';

                // Since there are minor differences between the lazy and greedy versions
                // We can just use the ternary operator and save some redundancy
                // @NOTE : Don't chain ternary operators together
                s = (peek_ch((*pattern)) == '?') ? create_state(S_NODE, data, NULL, a.start)
                                         : create_state(S_NODE, data, a.start, NULL);

                point_state_list(a.list, s);

                *fp++ = (peek_ch((*pattern)) == '?') ? create_fragment(s, create_state_list(&s->next1))
                                             : create_fragment(s, create_state_list(&s->next2));


                (*pattern) += (peek_ch((*pattern)) == '?') ? 1 : 0;
                fp = link_fragments(fp, (*pattern));
                (*pattern)++;
                regex_log("0 Or More: State %p, Node State\n", (void *) s);

                break;
            case '+':
                a = *--fp;
                data.ch = '\0';

                s = (peek_ch((*pattern)) == '?') ? create_state(S_NODE, data, NULL, a.start)
                                         : create_state(S_NODE, data, a.start, NULL);

                point_state_list(a.list, s);

                *fp++ = (peek_ch((*pattern)) == '?') ? create_fragment(a.start, create_state_list(&s->next1))
                                             : create_fragment(a.start, create_state_list(&s->next2));

                (*pattern) += (peek_ch((*pattern)) == '?') ? 1 : 0;
                fp = link_fragments(fp, (*pattern));
                (*pattern)++;
                regex_log("1 Or More: State %p, Node State\n", (void *) s);

                break;

            case '.':
                data.meta = M_ANY_CH;
                s = create_state(S_META_CH, data, NULL, NULL);
                *fp++ = create_fragment(s, create_state_list(&s->next1));

                fp = link_fragments(fp, (*pattern));
                regex_log("State %p, Type = %s, ch = %c\n",
                        (void *) s, state_type_to_string(s->type), s->data.ch);
                (*pattern)++;
                break;

            default: // Normal characters
                def:
                data.ch = *(*pattern);
                s = create_state(S_LITERAL_CH, data, NULL, NULL);
                *fp++ = create_fragment(s, create_state_list(&s->next1));

                // check if we should link this fragment with the one before it
                fp = link_fragments(fp, (*pattern));
                regex_log("State %p, Type = %s, ch = %c\n",
                        (void *) s, state_type_to_string(s->type), s->data.ch);
                (*pattern)++;
                break;
        } // Switch
    } // While


    return stack[0];
}

// Naviagtes the FSM and (should) returns the matched sub-string
static char *perform_regex(State *start, char *string) {
    regex_log("\n----- Navigating Finite State Machine -----\n");

    // Backtracking related
    BacktrackData *backtrack_stack = malloc(sizeof(BacktrackData) * MAX_STACK_SIZE);
    BacktrackData *btp = backtrack_stack;
    BacktrackData b;
    int do_backtrack = 0;

    // Capturing group related - This is the worst and only way I can imagine doing this
    // We capture upto 99 groups used to check whether we should be collecting
    CaptureGroupData *cgd = malloc(sizeof(CaptureGroupData));


    for (int i = 0; i < MAX_CAPTURE_GROUPS; i++) {
        // Defaulting values
        cgd->icg[i] = 0;
        cgd->str_ptrs[i] = 0;
        cgd->cgs[i][0] = '\0';
    }

    State *s = start; // This is the state we are checking
    char *rtn_str = malloc(sizeof(char) * MAX_STRING_SIZE); // This get's freed by the caller of regex()
    char *sp = rtn_str;

    while (1) {
        regex_log("State %p, ", (void *) s);

        switch (s->type) {
            case S_BACK_REFERENCE:
                // If we are inside the capture group we can't reference it e.g (ab\1)
                if (cgd->icg[s->data.ch - 1] == 1) {
                    regex_log("Regex engine runtime error: Can't backreference whilst inside the capture group\n");
                    return "";
                }

                // Start matching the string in cgs to the supplied string
                const char *brp = &cgd->cgs[s->data.ch - 1][0]; // Back reference pointer
                regex_log("Back reference matching\n");

                while (*brp == *string && *brp != '\0' && *string != '\0') {
                    regex_log("Literal character \"%c\" matched character \"%c\" in string\n", *brp, *string);
                    brp++;
                    capture_group_str_grab(cgd, *string);
                    *sp++ = *string++;
                }
                // Making the whole of the backreference has matched
                if (*brp != '\0') {
                    regex_log("Literal character \"%c\" did not match character \"%c\" in string\n",
                            *brp, *string);
                    do_backtrack = 1;
                } else if (s->next2)
                    *btp++ = create_backtrack_data(string, sp, s->next2, cgd);

                s = s->next1;
                break;

            case S_REVERSE_CCLASS:
                // If we get a match
                if (!match_ch_str(*string, s->data.cclass) && *string != '\0') {
                    regex_log("Reverse character class \"%s\" matched with character \"%c\" in string \n",
                           s->data.cclass, *string);

                    capture_group_str_grab(cgd, *string);
                    *sp++ = *string++;
                    if (s->next2)
                        *btp++ = create_backtrack_data(string, sp, s->next2, cgd);

                    s = s->next1;
                } else {
                    regex_log("Reverse character class \"%s\" did not match with character \"%c\" in string \n",
                           s->data.cclass, *string);
                    do_backtrack = 1;
                }
                break;

            case S_CCLASS:
                // If we get a match
                if (match_ch_str(*string, s->data.cclass)) {
                    regex_log("Character class \"%s\" matched with character \"%c\" in string \n",
                           s->data.cclass, *string);

                    capture_group_str_grab(cgd, *string);
                    *sp++ = *string++;
                    if (s->next2)
                        *btp++ = create_backtrack_data(string, sp, s->next2, cgd);

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

                            capture_group_str_grab(cgd, *string);
                            *sp++ = *string++;
                            if (s->next2)
                                *btp++ = create_backtrack_data(string, sp, s->next2, cgd);

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

                    capture_group_str_grab(cgd, *string);
                    *sp++ = *string++;
                    if (s->next2)
                        *btp++ = create_backtrack_data(string, sp, s->next2, cgd);
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
                regex_log("Match completed!\n\n");

                // Printing out the capturing groups for debugging
                for (int i = 0; i < 9; i++)
                    regex_log("capture_group %d - %s -\n", i+1, cgd->cgs[i]);
                regex_log("\n");
                *sp++ = '\0';

                // Cleanup
                //free(backtrack_stack);
                //free(cgd);
                return rtn_str;
            
            case S_NODE:
                regex_log("Node State, cg = %d, next1 = %p, next2 = %p\n",
                    s->data.cg, (void *) s->next1, (void *) s->next2);
                if (s->next2)
                    *btp++ = create_backtrack_data(string, sp, s->next2, cgd);
                s = s->next1;
                break;

            case S_CG_NODE:
                if (s->data.cg > 0) {
                    // We only collect once, so "(abc)+" shouldn't result in "abcabc" etc for the capture string
                    if (cgd->icg[s->data.cg - 1] == 0)
                        cgd->icg[s->data.cg - 1] = 1;
                } else if (s->data.cg < 0)
                    // 2 let's us know that we've already been here
                    cgd->icg[(s->data.cg * (-1)) - 1] = 2;

                regex_log("Capture Group Node State, cg = %d, next1 = %p, next2 = %p\n",
                    s->data.cg, (void *) s->next1, (void *) s->next2);
                if (s->next2)
                    *btp++ = create_backtrack_data(string, sp, s->next2, cgd);
                s = s->next1;
                break;

            case S_AQ_NODE:
                s->data.aq.visited += 1;
                if (s->data.aq.visited < s->data.aq.min) {
                    // Laziness isn't implemented yet so we assume that next1 links back correctly
                    // We don't store the backtracking data here because we are pretending next2
                    // Doesn't go anywhere
                    regex_log("Arbitrary Quantifier Node State, max = %u, min = %u, visited = %i ",
                            s->data.aq.max, s->data.aq.min, s->data.aq.visited);
                    regex_log("next1 = %p, next2 = %p\n", (void *) s->next1, (void *) s->next2);
                    regex_log("Below minimum\n");
                    s = (s->data.aq.lazy == 0) ? s->next1 : s->next2;

                } else if (s->data.aq.visited > s->data.aq.max) {
                    // If we go past we should backtrack
                    regex_log("Arbitrary Quantifier Node State, max = %u, min = %u, visited = %i ",
                            s->data.aq.max, s->data.aq.min, s->data.aq.visited);
                    regex_log("next1 = %p, next2 = %p\n", (void *) s->next1, (void *) s->next2);
                    regex_log("Went past maximum\n");
                    do_backtrack = 1;

                } else {
                    if (s->next2)
                        *btp++ = create_backtrack_data(string, sp, s->next2, cgd);
                    s = s->next1;
#if 0
                    if (s->data.aq.lazy == 0) {
                        if (s->next2)
                            *btp++ = create_backtrack_data(string, sp, s->next2, cgd);
                        s = s->next1;
                    } else {
                        if (s->next1)
                            *btp++ = create_backtrack_data(string, sp, s->next1, cgd);
                        s = s->next2;
                    }
#endif
                }
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
                // Replacing relevant data
                b = *--btp;
                string = b.string;
                sp = b.sp;
                s = b.s;
                regex_log("capture_group 1 before backtrack = %s\n", cgd->cgs[0]);
                *cgd = *b.cgd;

                regex_log("Backtrack complete: Starting at state %p\n\n", (void *) s);
                regex_log("capture_group 1 after backtrack = %s\n", cgd->cgs[0]);
                do_backtrack = 0;
            }
        }
    } // While
}

// Collects strings inside relevant capture groups
static void capture_group_str_grab(CaptureGroupData *cgd, char ch) {

    for (int i = 0; i < MAX_CAPTURE_GROUPS; i++) {
        if (cgd->icg[i] == 1) {
            cgd->cgs[i][cgd->str_ptrs[i]++] = ch;
            cgd->cgs[i][cgd->str_ptrs[i]] = '\0';
        }
    }
}


// Returns 0 if the pattern is correct
static int check_pattern_correctness(char *pattern) {
    int a = 0;
    a += state_altering_check(pattern);
    a += character_class_check(pattern);
    return a;
}

/**
 * State altering characters can't be next to each other or at the start of the pattern
 * e.g "ab?+" "+ab"
 */
static int state_altering_check(char *p) {
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

static int character_class_check(char *p) {
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
            if (*c == '-' && *(c - 1) != '\\' && *(c - 2) != '-') { // Checking if it's escaped and not intruding
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
static Fragment *link_fragments(Fragment *fp, char *sp) {
    Fragment a, b;
    int tmpa, tmpb;
    char *tmpc = sp;
    if (peek_ch(sp) == '{') {// Check if it's a quantifier
        if (get_arbitrary_quantifier(&tmpc, &tmpa, &tmpb) == 1) {
            regex_log("we in here\n");
            b = *--fp;
            a = *--fp;
            point_state_list(a.list, b.start);
            *fp++ = create_fragment(a.start, b.list);
        }

    } else if (match_ch_str(peek_ch(sp), "?+*") == 0) {
        b = *--fp;
        a = *--fp;
        point_state_list(a.list, b.start);
        *fp++ = create_fragment(a.start, b.list);
    }

    return fp;
}

static State *create_state(StateType type, StateData data, State * const next1, State * const next2) {
    State *a = malloc(sizeof(State));
    *rfp++ = a;
    a->type  = type;
    a->data  = data;
    a->next1 = next1;
    a->next2 = next2;

    return a;
}

static Fragment create_fragment(State *s, StateList *l) {
    Fragment rtn;
    rtn.start = s;
    rtn.list = l;
    return rtn;
}

// The pointers from the first fragment are pointed the the start of the next one
static void point_state_list(StateList *l, State *a) {
    for (int i = 0; i < l->n; i++) {
        *(l->l)[i] = a;
    }
}

static StateList *create_state_list(State **first) {
    StateList *l = malloc(sizeof(StateList));
    *rfp++ = l;
    l->n = 0;
    l->l[l->n++] = first;

    return l;
}

static StateList *append_lists(StateList *a, StateList *b) {
    StateList *rtn = malloc(sizeof(StateList));
    *rfp++ = rtn;
    rtn->n = 0;

    for (int i = 0; i < a->n; i++)
        rtn->l[i] = a->l[i];

    for (int i = 0; i < b->n; i++)
        rtn->l[i + a->n] = b->l[i];

    rtn->n = a->n + b->n;

    return rtn;
}


static char *create_character_class(char *sp, StateData *data) {
    data->cclass = malloc(sizeof(char) * MAX_STRING_SIZE);
    *rfp++ = data->cclass;
    char *cp = &data->cclass[0];
    // @TODO: We don't look for backwards slashes right now

    while (*sp != ']') {
        if (*sp == '-') {
            if (*(sp - 1) != '\\'   // making sure it's not escaped
             && *(sp - 1) != '['    // checking we aren't at the start
             && peek_ch(sp) != '\\' // more escape checking
             && peek_ch(sp) != ']'  // Checking we aren't at the end
             && *(sp - 2) != '-') { // Making sure we aren't intruding on another range e.g A-Z-a
                // We've already collected the first character
                for (int i = *(sp - 1) + 1; i <= peek_ch(sp); i++)
                    *cp++ = i;

                sp += 2;
            } else {
                *cp++ = *sp++;
            }

        } else if (*sp == '\\') {
            State *s = parse_escapes(&sp); // Utilizing parse_escapes
            switch (s->type) {
                case S_REVERSE_CCLASS:
                    regex_log("reverse cclass\n");
                    for (char i = 32; i < 127; i++) { // We skip the first lot that we definetly don't match
                        if (match_ch_str(i, s->data.cclass) == 0) { // We take the char if it isn't in the string
                            *cp++ = i;
                        }
                    }
                    // Matching specific stuff we skipped
                    if (match_ch_str(9 , s->data.cclass) == 0) *cp++ = 9;  // tabs
                    if (match_ch_str(10, s->data.cclass) == 0) *cp++ = 10; // newline 
                    if (match_ch_str(13, s->data.cclass) == 0) *cp++ = 13; // carriage return 

                    sp++;
                    break;

                case S_CCLASS:
                    // We can combine the classes together
                    ;
                    char *cclass_p = &s->data.cclass[0];
                    while (*cclass_p != '\0') *cp++ = *cclass_p++;
                    sp++;
                    break;

                case S_LITERAL_CH:
                    *cp++ = s->data.ch;
                    sp++;
                    break;

                default:
                    regex_log("Unhandled state type \"%s\" in function %s\n",
                           state_type_to_string(s->type), __func__);
                    break;
            }
        }
        else // collect as normal
        *cp++ = *sp++; 
            
    }

    *cp++ = '\0';

    return sp;
}

static State *parse_escapes(char **p) {
    State *s;
    StateData data;

    regex_log("Parsing escaped character(s)\n");

    switch (peek_ch(*p)) {
        case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': 
            ;
            int backref;

            if (peek_ch((*p) + 1) >= '0' && peek_ch((*p) + 1) <= '9') { // We accept back refs upto 99
                backref = (peek_ch(*p) - 48) * 10;
                backref += peek_ch((*p) + 1) - 48;
                regex_log("ref no. %d\n", backref);
                *p = (*p) + 2;
            } else {
                backref = peek_ch(*p) - 48;
                regex_log("ref no. %d\n", backref);
                *p = (*p) + 1;
            }

            data.ch = backref;
            s = create_state(S_BACK_REFERENCE, data, NULL, NULL);
            regex_log("Back reference: State %p, Node State\n", (void *) s);
            break;

        // White space
        case 'n': case 't': case 'r':
            data.ch = peek_ch(*p) == 'n' ? '\n' : peek_ch(*p) == 't' ? '\t' : '\r';
            s = create_state(S_LITERAL_CH, data, NULL, NULL);
            regex_log("State %p, Type = %s, ch = %c\n",
                    (void *) s, state_type_to_string(s->type), s->data.ch);
            (*p)++;
            break;
            
        case 'S':
            create_character_class("\n\t\r ]", &data);
            s = create_state(S_REVERSE_CCLASS, data, NULL, NULL);
            regex_log("Shorthand \"\\s\" (NOT any whitespace character) Parsed\n");
            regex_log("State %p, Type = %s, range = %s\n",
                    (void *) s, state_type_to_string(s->type), s->data.cclass);
            (*p)++;
            break;

        case 's':
            create_character_class("\n\t\r ]", &data);
            s = create_state(S_CCLASS, data, NULL, NULL);
            regex_log("Shorthand \"\\s\" (Any whitespace character) Parsed\n");
            regex_log("State %p, Type = %s, range = %s\n",
                    (void *) s, state_type_to_string(s->type), s->data.cclass);
            (*p)++;
            break;

        case 'D': // NOT any digit aka [0-9]
            create_character_class("0-9]", &data);
            s = create_state(S_REVERSE_CCLASS, data, NULL, NULL);
            regex_log("Shorthand \"\\D\" (NOT any digit) Parsed\n");
            regex_log("State %p, Type = %s, range = %s\n",
                    (void *) s, state_type_to_string(s->type), s->data.cclass);
            (*p)++;
            break;

        case 'd': // Any digit aka [0-9]
            create_character_class("0-9]", &data);
            s = create_state(S_CCLASS, data, NULL, NULL);
            regex_log("Shorthand \"\\d\" (any digit) Parsed\n");
            regex_log("State %p, Type = %s, range = %s\n",
                    (void *) s, state_type_to_string(s->type), s->data.cclass);
            (*p)++;
            break;

        case 'W': // NOT any alphanumeric char and underscore aka [^A-Za-z0-9_]
            create_character_class("A-Za-z0-9_]", &data);
            s = create_state(S_REVERSE_CCLASS, data, NULL, NULL);
            regex_log("Shorthand \"\\W\" (NOT any alphanum + underscore) Parsed\n");
            regex_log("State %p, Type = %s, range = %s\n",
                    (void *) s, state_type_to_string(s->type), s->data.cclass);
            (*p)++;
            break;

        case 'w': // Any alphanumeric char and underscore aka [A-Za-z0-9_]
            create_character_class("A-Za-z0-9_]", &data);
            s = create_state(S_CCLASS, data, NULL, NULL);
            regex_log("Shorthand \"\\w\" (any alphanum + underscore) Parsed\n");
            regex_log("State %p, Type = %s, range = %s\n",
                    (void *) s, state_type_to_string(s->type), s->data.cclass);
            (*p)++;
            break;

        case 'A':
            regex_log("\\A (beginning of string) not implemented yet");
            goto def;

        case 'z': case 'Z':
            regex_log("End of string not implemented yet\n");
            goto def;

        case 'b': case 'B':
            regex_log("Word boundaries not implemented yet\n");
            goto def;

        default:
        def:// Anything not specially handled gets returned as a literal character
            data.ch = peek_ch(*p);
            s = create_state(S_LITERAL_CH, data, NULL, NULL);
            regex_log("State %p, Type = %s, ch = %c\n",
                    (void *) s, state_type_to_string(s->type), s->data.ch);
            (*p)++;
            break;
    }
    return s;
}

// Returns 1 on success
static int get_arbitrary_quantifier(char **p, int *a, int *b) {
    char *pp = *p + 1; // Jump inside the brace

    char a_str[MAX_STRING_SIZE];
    char b_str[MAX_STRING_SIZE];
    char *str_ptr = &a_str[0];

    while (match_ch_str(*pp, "0123456789") == 1) *str_ptr++ = *pp++;

    if (*pp == '}') { // exact quantifier e.g {2}
        *a = atoi(a_str);
        *b = EXACT_QUANTIFIER;

    } else if (*pp == ',') {

        if (*(pp + 1) == '}') { // open ended quantifier e.g {2,}
            *a = atoi(a_str);
            *b = OPEN_ENDED_QUANTIFIER;
            pp++;

        } else { // arbitrary quantifier e.g {2,4}
            // Move past comma
            pp++;
            str_ptr = b_str;
            while (match_ch_str(*pp, "0123456789") == 1) *str_ptr++ = *pp++;
            if (*pp == '}') {
                *a = atoi(a_str);
                *b = atoi(b_str);

            } else { // Messed up
                return 0;
            }
        }
    } else { // Messed up
        return 0;
    }
    *p = pp; // We should end on the closing brace
    regex_log("debuggy stuff\n");
    regex_log("a %d b %d p %c\n", *a, *b, **p);
    return 1;
}


static BacktrackData create_backtrack_data(char *string, char *sp, State *s, CaptureGroupData *cgd) {
    BacktrackData rtn;
    rtn.string = string;
    rtn.sp = sp;
    rtn.s = s;

    rtn.cgd = malloc(sizeof(CaptureGroupData));
    *rfp++ = rtn.cgd;
    *rtn.cgd = *cgd;
    return rtn;
}

static char *state_type_to_string(StateType type) {
    switch (type) {
        case S_FINAL: return "S_FINAL";
        case S_NODE: return "S_NODE";
        case S_LITERAL_CH: return "S_LITERAL_CH";
        case S_META_CH: return "S_META_CH";
        case S_CCLASS: return "S_CCLASS";
        case S_REVERSE_CCLASS: return "S_REVERSE_CCLASS";
        case S_BACK_REFERENCE: return "S_BACK_REFERENCE";
        default: return "Unhandled case in state_type_to_string";
    }
}

#if 0
static char *meta_ch_type_to_string(MetaChType type) {
    switch (type) {
        case M_ANY_CH: return "M_ANY_CH";
        default: return "Unhandled case in meta_ch_type_to_string";
    }
}
#endif

// returns 1 in the character is in the string
static int match_ch_str(char ch, char *str) {
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

static inline char reverse_peek_ch(char *str) {
    return *(str-1);
}

// Allows nice printing showing where the error is in the pattern
static void pattern_error(char *p, unsigned int pos, unsigned int range, char *msg, ...) {
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
static void regex_log(char *msg, ...) {
    if (options.suppress_logging) return;

    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
}

// Some options can be handled as soon as we enter the function
static void handle_options(unsigned int opts) {
    options.suppress_logging = (opts & REGEX_SUPPRESS_LOGGING) ? 1 : 0;
    options.start_of_string = 0;
}
