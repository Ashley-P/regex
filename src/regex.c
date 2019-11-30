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
    T_LITERAL_CH  = 0x1,
    T_META_CH     = 0x10, // For non state altering meta characters e.g '.'
    T_FINAL       = 0x100,

    T_GREEDY_PLUS = 0x1000, // '+'
    T_LAZY_PLUS   = 0x10000,   // '+?'

    T_GREEDY_STAR = 0x100000,
    T_LAZY_STAR   = 0x1000000,

    T_STATE_ALTERING = T_GREEDY_PLUS | T_LAZY_PLUS | T_GREEDY_STAR | T_LAZY_STAR,
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





/***** Function Prototypes *****/
Token *tokenize_pattern(char *pattern);
State *parse_tokens(Token *tokens);
Fragment *link_fragments(Fragment *fp, Token *tokens);

char *perform_regex(State *start, char *string);

/***** Utility functions *****/
State *create_state(StateType type, StateData data, State * const next1, State * const next2);
Fragment create_fragment(State *s, StateList *l);
void point_state_list(StateList *l, State *a);
StateList *create_state_list(State **first);
StateList *append_lists(StateList *a, StateList *b);

BacktrackData create_backtrack_data(char *string, char *sp, State *s);

char *state_type_to_string(StateType type);
char *token_type_to_string(TokenType type);
char *meta_ch_type_to_string(MetaChType type);




char *regex(char *pattern, char *string) {
    // Echoing back for no real reason
    printf("Pattern : \"%s\"\n", pattern);
    printf("String  : \"%s\"\n", string);

    // We'd do some checking here maybe
    Token *tokens = tokenize_pattern(pattern);
    // We'd do some checking here maybe
    State *start = parse_tokens(tokens);

    // Do the regex at each point of the string
    char *return_str = "";
    for (int i = 0; *(string + i) != '\0'; i++) {
        printf("\n\nRegex Iteration %d\n", i + 1);
        return_str = perform_regex(start, string + i);
        if (*return_str != '\0')
            break;
        else
            printf("\nIteration %d failed\n\n", i + 1);
    }

    if (*return_str == '\0')
        printf("Regex Failed\n");
    else
        printf("Returned string : %s\n\n", return_str);
    return return_str;
}


Token *tokenize_pattern(char *pattern) {
    printf("\n----- Tokenizing pattern -----\n");
    Token *tokens = malloc(sizeof(Token) * MAX_BUFFER_SIZE);
    Token *tp = tokens;
    Token t;

    while (*pattern != '\0') {
        switch (*pattern) {
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
        printf("TokenType = %s, ch = %c\n", token_type_to_string((tp - 1)->type), (tp - 1)->ch);
    }

    t.type = T_FINAL;
    t.ch = '\0';
    *tp++ = t;

    printf("Tokenizing complete\n");
    return tokens;
}


State *parse_tokens(Token *tokens) {
    printf("\n----- Parsing tokens -----\n");

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
            case T_GREEDY_STAR:
                a = *--fp;
                data.ch = '\0';

                s = create_state(S_NODE, data, a.start, NULL);
                point_state_list(a.list, s);
                *fp++ = create_fragment(s, create_state_list(&s->next2));
                fp = link_fragments(fp, tokens);
                tokens++;
                printf("State %p, Node State\n", (void *) s);
                break;

            case T_LAZY_STAR:
                a = *--fp;
                data.ch = '\0';

                s = create_state(S_NODE, data, NULL, a.start);
                point_state_list(a.list, s);
                *fp++ = create_fragment(s, create_state_list(&s->next1));
                fp = link_fragments(fp, tokens);
                tokens++;
                printf("State %p, Node State\n", (void *) s);
                break;

            case T_GREEDY_PLUS:
                a = *--fp;
                data.ch = '\0';

                s = create_state(S_NODE, data, a.start, NULL);
                point_state_list(a.list, s);
                *fp++ = create_fragment(a.start, create_state_list(&s->next2));
                fp = link_fragments(fp, tokens);
                tokens++;
                printf("State %p, Node State\n", (void *) s);
                break;

            case T_LAZY_PLUS:
                a = *--fp;
                data.ch = '\0';

                s = create_state(S_NODE, data, NULL, a.start);
                point_state_list(a.list, s);
                *fp++ = create_fragment(a.start, create_state_list(&s->next1));
                fp = link_fragments(fp, tokens);
                tokens++;
                printf("State %p, Node State\n", (void *) s);
                break;

            case T_META_CH:
                data.meta = M_ANY_CH;
                s = create_state(S_META_CH, data, NULL, NULL);
                *fp++ = create_fragment(s, create_state_list(&s->next1));

                printf("State %p, Type = %s, ch = %s\n",
                        (void *) s, state_type_to_string(s->type), meta_ch_type_to_string(s->data.meta));

                fp = link_fragments(fp, tokens);
                tokens++;
                break;

            case T_LITERAL_CH:
                data.ch = tokens->ch;
                s = create_state(S_LITERAL_CH, data, NULL, NULL);
                *fp++ = create_fragment(s, create_state_list(&s->next1));

                printf("State %p, Type = %s, ch = %c\n",
                        (void *) s, state_type_to_string(s->type), s->data.ch);

                // Concatanation of the top 2 fragments on the stack happen if the next token isn't
                // one that alters the flow of the FSM
                fp = link_fragments(fp, tokens);
                tokens++;
                break;
            default:
                printf("nani\n");
                tokens++;
                break;
        }
    }

    // If we aren't left with one fragment at this point something has gone wrong
    if (fragments != fp-1) {
        printf("Uh oh\n");
        exit(0);
    }

    // Adding the final state
    data.ch = '\0';
    s = create_state(S_FINAL, data, NULL, NULL);
    point_state_list((*--fp).list, s);

    printf("Parsing complete\n");

    return start;
}

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

// Naviagtes the FSM and (should) returns the matched sub-string
char *perform_regex(State *start, char *string) {
    printf("\n----- Navigating Finite State Machine -----\n");

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
        printf("State %p, ", (void *) s);

        switch (s->type) {
            case S_META_CH:

                switch (s->data.meta) {
                    // Matches anything but the null character
                    case M_ANY_CH:
                        if (*string != '\0') {
                            printf("Meta Character \".\" matched character \"%c\" in string \n", *string);

                            *sp++ = *string++;
                            if (s->next2)
                                *btp++ = create_backtrack_data(string, sp, s->next2);

                            s = s->next1;
                        } else {
                            printf("Meta Character \".\" did not match character \"%c\" in string \n", *string);
                            do_backtrack = 1;
                        }
                        break;
                    default:
                        printf("Shouldn't hit this\n");
                        return "";
                }

                break;

            case S_LITERAL_CH:
                if (s->data.ch == *string) {
                    printf("Literal character \"%c\" matched character \"%c\" in string\n",
                            s->data.ch, *string);

                    *sp++ = *string++;
                    if (s->next2)
                        *btp++ = create_backtrack_data(string, sp, s->next2);
                    s = s->next1;
                } else {
                    // Backtracking would happen here
                    printf("Literal character \"%c\" did not match character \"%c\" in string\n",
                            s->data.ch, *string);
                    do_backtrack = 1;
                }
                break;

            // Specials
            case S_FINAL:
                printf("Match completed!\n");
                *sp++ = '\0';
                return rtn_str;
            
            case S_NODE:
                printf("Node State, next1 = %p, next2 = %p\n", (void *) s->next1, (void *) s->next2);
                if (s->next2)
                    *btp++ = create_backtrack_data(string, sp, s->next2);
                s = s->next1;
                break;
            default:
                printf("Shouldn't hit this\n");
                return "";
        } // Switch

        if (do_backtrack) {
            printf("\nAttempting to backtrack\n");

            if (backtrack_stack == btp) {
                printf("Stack empty, unable to backtrack\n");
                return "";
            } else {
                b = *--btp;
                string = b.string;
                sp = b.sp;
                s = b.s;
                printf("Backtrack complete: Starting at state %p\n\n", (void *) s);
                do_backtrack = 0;
            }
        }
    } // While
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

    for (int i = 0; i < a->n; i++)
        rtn->l[i] = a->l[i];

    for (int i = 0; i < b->n; i++)
        rtn->l[i + rtn->n] = a->l[i];

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
        default: return "Unhandled case in token_type_to_string";
    }
}

char *meta_ch_type_to_string(MetaChType type) {
    switch (type) {
        case M_ANY_CH: return "M_ANY_CH";
        default: return "Unhandled case in meta_ch_type_to_string";
    }
}
