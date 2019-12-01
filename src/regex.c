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
#include "regex_utils.h"



/***** Defines *****/
#define MAX_BUFFER_SIZE 64


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
    S_START = 1,
    S_FINAL,
    S_NODE,

    // Normal States
    S_LITERAL_CH,
    S_META_CH,
    S_RANGE,
} StateType;

typedef union StateData_ {
    char ch; // for literal characters
    MetaChType meta; // Meta character types
} StateData;

typedef struct State_ {
    StateType type;

    union StateData_ data;

    struct State_ *next1;
    struct State_ *next2;
} State;


typedef struct StateList_ {
    struct State_ **l[MAX_BUFFER_SIZE];
    int n;
} StateList;

// Fragments of the state machine
typedef struct Fragment_ {
    struct State_ *start;
    struct StateList_ *list;
} Fragment;


typedef enum {
    T_LITERAL_CH   = 1 << 0,
    T_META_CH      = 1 << 1,  // For non state altering meta characters e.g '.'
    T_FINAL        = 1 << 2,
    
    T_GREEDY_PLUS  = 1 << 3, // '+'
    T_LAZY_PLUS    = 1 << 4, // '+?'

    T_GREEDY_STAR  = 1 << 5, // '*'
    T_LAZY_STAR    = 1 << 6, // '*?'

    T_GREEDY_QMARK = 1 << 7, // '?'
    T_LAZY_QMARK   = 1 << 8, // '??'

    T_STATE_ALTERING = T_GREEDY_PLUS | T_LAZY_PLUS |
                       T_GREEDY_STAR | T_LAZY_STAR |
                       T_GREEDY_QMARK | T_LAZY_QMARK,
#if 0
    // Parentheses
    T_OPEN_P,
    T_CLOSE_P,

    // Square brackets
    T_OPEN_SB,
    T_CLOSE_SB,
#endif
} TokenType;

typedef struct Token_ {
    TokenType type;
    char ch;
} Token;

typedef struct BacktrackData_ {
    char *string;
    char *sp;
    State *s;
} BacktrackData;


/***** Constants *****/
static int surpress_logging = 0;





/***** Function Prototypes *****/
char *regex(char *pattern, char *string, unsigned int options);
Token *tokenize_pattern(char *pattern);
State *parse_tokens(Token *tokens);
char *perform_regex(State *start, char *string);

int check_pattern_correctness(char *pattern);
int state_altering_check(char *p);

int check_tokens_correctness(Token *tokens);


/***** Utility functions *****/
Fragment *link_fragments(Fragment *fp, Token *tokens);
State *create_state(StateType type, StateData data, State * const next1, State * const next2);
Fragment create_fragment(State *s, StateList *l);
void point_state_list(StateList *l, State *a);
StateList *create_state_list(State **first);
StateList *append_lists(StateList *a, StateList *b);

BacktrackData create_backtrack_data(char *string, char *sp, State *s);

char *state_type_to_string(StateType type);
char *token_type_to_string(TokenType type);
char *meta_ch_type_to_string(MetaChType type);

void pattern_error(char *p, unsigned int pos, char *msg, ...);
void regex_log(char *msg, ...);



char *regex(char *pattern, char *string, unsigned int options) {
    // Handle options - should be moved to it's own function
    if (options & REGEX_SURPRESS_LOGGING) surpress_logging = 1;

    // Echoing back for no real reason
    regex_log("Pattern : \"%s\"\n", pattern);
    regex_log("String  : \"%s\"\n", string);

    // We'd do some checking here maybe
    if (!check_pattern_correctness(pattern))
        return "";

    Token *tokens = tokenize_pattern(pattern);

    if (!check_tokens_correctness(tokens))
        return "";

    // We'd do some checking here maybe
    State *start = parse_tokens(tokens);

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


Token *tokenize_pattern(char *pattern) {
    regex_log("\n----- Tokenizing pattern -----\n");
    Token *tokens = malloc(sizeof(Token) * MAX_BUFFER_SIZE);
    Token *tp = tokens;
    Token t;

    while (*pattern != '\0') {
        switch (*pattern) {
            case '?':
                // Peek the next character to determine whether it's lazy or greedy
                if (*(pattern + 1) == '?') {
                    t.type = T_LAZY_QMARK;
                    pattern++;
                } else
                    t.type = T_GREEDY_QMARK;

                t.ch = '?';
                *tp++ = t;
                pattern++;
                break;

            case '*':
                // Peek the next character to determine whether it's lazy or greedy
                if (*(pattern + 1) == '?') {
                    t.type = T_LAZY_STAR;
                    pattern++;
                } else
                    t.type = T_GREEDY_STAR;

                t.ch = '*';
                *tp++ = t;
                pattern++;
                break;

            case '+':
                // Peek the next character to determine whether it's lazy or greedy
                if (*(pattern + 1) == '?') {
                    t.type = T_LAZY_PLUS;
                    pattern++;
                } else
                    t.type = T_GREEDY_PLUS;

                t.ch = '+';
                *tp++ = t;
                pattern++;
                break;

            case '.':
                t.type = T_META_CH;
                t.ch = *pattern++;
                *tp++ = t;
                break;

            // Literal Characters
            default:
                t.type = T_LITERAL_CH;
                t.ch = *pattern++;
                *tp++ = t;
                break;
        }
        regex_log("TokenType = %s, ch = %c\n", token_type_to_string((tp - 1)->type), (tp - 1)->ch);
    }

    t.type = T_FINAL;
    t.ch = '\0';
    *tp++ = t;

    regex_log("Tokenizing complete\n");
    return tokens;
}


State *parse_tokens(Token *tokens) {
    regex_log("\n----- Parsing tokens -----\n");

    Fragment fragments[MAX_BUFFER_SIZE];
    Fragment *fp = fragments;
    Fragment a, b;

    StateData data;
    data.ch = '\0';
    State *start = create_state(S_START, data, NULL, NULL);
    State *s;

    for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
        fragments[i] = create_fragment(NULL, NULL);
    }

    *fp++ = create_fragment(start, create_state_list(&start->next1));

    while (tokens->type != T_FINAL) {
        switch (tokens->type) { 
            case T_GREEDY_QMARK: case T_LAZY_QMARK:
                a = *--fp;
                data.ch = '\0';

                s = (tokens->type == T_LAZY_QMARK) ? create_state(S_NODE, data, NULL, a.start)
                                                   : create_state(S_NODE, data, a.start, NULL);

                //point_state_list(a.list, s);

                *fp++ = (tokens->type == T_LAZY_QMARK)
                          ? create_fragment(s, append_lists(a.list, create_state_list(&s->next1)))
                          : create_fragment(s, append_lists(a.list, create_state_list(&s->next2)));

                fp = link_fragments(fp, tokens);
                tokens++;
                regex_log("State %p, Node State\n", (void *) s);
                break;

            case T_GREEDY_STAR: case T_LAZY_STAR:
                a = *--fp;
                data.ch = '\0';

                // Since there are minor differences between the lazy and greedy versions
                // We can just use the ternary operator and save some redundancy
                // @NOTE : Don't chain ternary operators together
                s = (tokens->type == T_LAZY_STAR) ? create_state(S_NODE, data, NULL, a.start)
                                                  : create_state(S_NODE, data, a.start, NULL);

                point_state_list(a.list, s);

                *fp++ = (tokens->type == T_LAZY_STAR) ? create_fragment(s, create_state_list(&s->next1))
                                                      : create_fragment(s, create_state_list(&s->next2));
                fp = link_fragments(fp, tokens);
                tokens++;
                regex_log("State %p, Node State\n", (void *) s);
                break;

            case T_GREEDY_PLUS: case T_LAZY_PLUS:
                a = *--fp;
                data.ch = '\0';

                s = (tokens->type == T_LAZY_PLUS) ? create_state(S_NODE, data, NULL, a.start)
                                                  : create_state(S_NODE, data, a.start, NULL);

                point_state_list(a.list, s);

                *fp++ = (tokens->type == T_LAZY_PLUS) ? create_fragment(a.start, create_state_list(&s->next1))
                                                      : create_fragment(a.start, create_state_list(&s->next2));
                fp = link_fragments(fp, tokens);
                tokens++;
                regex_log("State %p, Node State\n", (void *) s);
                break;

            case T_META_CH:
                data.meta = M_ANY_CH;
                s = create_state(S_META_CH, data, NULL, NULL);
                *fp++ = create_fragment(s, create_state_list(&s->next1));

                regex_log("State %p, Type = %s, ch = %s\n",
                        (void *) s, state_type_to_string(s->type), meta_ch_type_to_string(s->data.meta));

                fp = link_fragments(fp, tokens);
                tokens++;
                break;

            case T_LITERAL_CH:
                data.ch = tokens->ch;
                s = create_state(S_LITERAL_CH, data, NULL, NULL);
                *fp++ = create_fragment(s, create_state_list(&s->next1));

                regex_log("State %p, Type = %s, ch = %c\n",
                        (void *) s, state_type_to_string(s->type), s->data.ch);

                // Concatanation of the top 2 fragments on the stack happen if the next token isn't
                // one that alters the flow of the FSM
                fp = link_fragments(fp, tokens);
                tokens++;
                break;
            default:
                regex_log("nani\n");
                tokens++;
                break;
        }
    }

    // If we aren't left with one fragment at this point something has gone wrong
#if 0
    if (fragments != fp-1) {
        regex_log("Uh oh\n");
        exit(0);
    }
#endif
    //fp = link_fragments(fp, tokens);

    // Adding the final state
    data.ch = '\0';
    s = create_state(S_FINAL, data, NULL, NULL);
    point_state_list((*--fp).list, s);

    regex_log("Parsing complete\n");

    return start;
}

// Naviagtes the FSM and (should) returns the matched sub-string
char *perform_regex(State *start, char *string) {
    regex_log("\n----- Navigating Finite State Machine -----\n");

    BacktrackData backtrack_stack[MAX_BUFFER_SIZE];
    BacktrackData *btp = backtrack_stack;
    BacktrackData b;
    int do_backtrack = 0;

    State *s = start->next1; // This is the state we are checking
    char *rtn_str = malloc(sizeof(char) * MAX_BUFFER_SIZE);
    char *sp = rtn_str;

    if (start->next2)
        *btp++ = create_backtrack_data(string, sp, start->next2);


    while (1) {
        regex_log("State %p, ", (void *) s);

        switch (s->type) {
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


// Returns 1 if the pattern is correct
int check_pattern_correctness(char *pattern) {
    int a = 1;
    a = state_altering_check(pattern);
    return a;
}

/**
 * State altering tokens can't be next to each other or at the start of the pattern
 * e.g "ab?+" "+ab"
 */
int state_altering_check(char *p) {
    // Checking the beginning of the token stream
    int pp = 0; // Pattern pointer

    switch (*p) {
        case '*': case '+': case '?':
            pattern_error(p, 0, "Meta character \"%c\" is not allowed at the start of the pattern\n", *p);
            return 0;
        default:
            break;
    }

    while (*(p + pp) != '\0') {
        // Checking if tokens are next to each other
        switch (*(p + pp)) {
            case '*': case '+': case '?':
                // Chceking next character
                switch (*(p + pp + 1)) {
                    case '*': case '+':
                        pattern_error(p, pp + 1,
                                "Meta character \"%c\" is not allowed after meta character \"%c\"\n",
                                *(p + pp + 1), *(p + pp));
                        return 0;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
        pp++;
    }

    return 1;
}


// Returns 1 if the token stream is correct
int check_tokens_correctness(Token *tokens) {
    int a = 1;
    return a;
}


/***** Utility functions *****/
Fragment *link_fragments(Fragment *fp, Token *tokens) {
    // Peeking the next token
    if (!((tokens + 1)->type & T_STATE_ALTERING)) {
        Fragment b = *--fp;
        Fragment a = *--fp;
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

BacktrackData create_backtrack_data(char *string, char *sp, State *s) {
    BacktrackData rtn;
    rtn.string = string;
    rtn.sp = sp;
    rtn.s = s;

    return rtn;
}

char *state_type_to_string(StateType type) {
    switch (type) {
        case S_START: return "S_START";
        case S_FINAL: return "S_FINAL";
        case S_NODE: return "S_NODE";
        case S_LITERAL_CH: return "S_LITERAL_CH";
        case S_META_CH: return "S_META_CH";
        case S_RANGE: return "S_RANGE";
        default: return "Unhandled case in state_type_to_string";
    }
}

char *token_type_to_string(TokenType type) {
    switch (type) {
        case T_LITERAL_CH: return "T_LITERAL_CH";
        case T_META_CH: return "T_META_CH";
        case T_FINAL: return "T_FINAL";
        case T_GREEDY_PLUS: return "T_GREEDY_PLUS";
        case T_LAZY_PLUS: return "T_LAZY_PLUS";
        case T_GREEDY_STAR: return "T_GREEDY_STAR";
        case T_LAZY_STAR: return "T_LAZY_STAR";
        case T_GREEDY_QMARK: return "T_GREEDY_QMARK";
        case T_LAZY_QMARK: return "T_LAZY_QMARK";
        default: return "Unhandled case in token_type_to_string";
    }
}

char *meta_ch_type_to_string(MetaChType type) {
    switch (type) {
        case M_ANY_CH: return "M_ANY_CH";
        default: return "Unhandled case in meta_ch_type_to_string";
    }
}

// Allows nice printing showing where the error is in the pattern
void pattern_error(char *p, unsigned int pos, char *msg, ...) {
    printf("Error in pattern -> %s\n", p);

    // Constructing the string that points to the error in the pattern
    char str[MAX_BUFFER_SIZE];
    char *sp = str;
    for (unsigned int i = 0; i < 20 + pos; i++) {
        *sp++ = ' ';
    }
    *sp++ = '^';
    *sp++ = '\n';
    *sp++ = '\0';
    printf(str);

    // Printing the message
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
}

// Let's us easily surpress printing to the screen probably temporary
void regex_log(char *msg, ...) {
    if (surpress_logging) return;

    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
}
