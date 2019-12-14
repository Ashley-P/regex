#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "regex.h"

#define TEST_MAX_STRING_SIZE 1000

// This file incldues some automated tests
static int test_num = 1;
static char default_fp[] = "./bin/resources/automated.txt";


/***** Function Prototypes *****/
int run_test(char *pattern, char *string, char *match);
int parse_line(FILE *f, char *p, char *s, char *m);
char *collect_string_in_quotes(char *c, char *strp);
int count_quotations(char *s);



int main(int argc, char *argv[]) {
    // We expect a filepath, but if there is nothing then we just use a default one
    FILE *f;

    if (argc != 2) {
        printf("Incorrect argument number : expected 2 got %d\n", argc);
        printf("Defaulting to filepath \"%s\"\n", default_fp);
        f = fopen(default_fp, "r");
    } else {
        printf("Attempting to open file \"%s\"\n", argv[1]);
        f = fopen(argv[1], "r");
    }

    if (!f && argc == 2) {
        printf("Couldn't open file \"%s\"\n", argv[1]);
        printf("NOTE: Run the automated test from the project directory");
        exit(0);
    } else if (!f && argc != 2) {
        printf("Couldn't open file \"%s\"\n", default_fp);
        printf("NOTE: Run the automated test from the project directory");
        exit(0);
    } else
        printf("File successfully opened!\n\n");

    char pattern[TEST_MAX_STRING_SIZE];
    char string[TEST_MAX_STRING_SIZE];
    char match[TEST_MAX_STRING_SIZE];
    //char line[TEST_MAX_STRING_SIZE];
    int i;
    do {
        i = parse_line(f, pattern, string, match);
        //printf("Pattern -> \"%s\" : String -> \"%s\" : Match -> \"%s\"\n", pattern, string, match);
        if (i == 0)
            break;
        else if (i == 1) {
            assert(run_test(pattern, string, match));
            test_num++;
        } else if (i == 2)
            continue;
    } while (i != 0);
    // We don't check whether the test files are correct so don't be wrong
#if 0
    int test_num = 0;
    while (fscanf(f, "%s%s%s", pattern, string, match) != EOF) {
        assert(run_test(pattern, string, match));
        test_num++;
    }
#endif
    // If we get here then everything is complete
    printf("\n%d tests completed successfully!\n", test_num - 1);
}

// returns 1 if the value is what it should be
int run_test(char *pattern, char *string, char *match) {
    printf("----- Test %d -----\n", test_num);
    printf("Pattern -> \"%s\" : String -> \"%s\" : Match -> \"%s\"\n", pattern, string, match);
    char *str;
    str = regex(pattern, string, REGEX_SUPPRESS_LOGGING);
    printf("%s -> %s\n\n", pattern, str);
    if (!strcmp(str, match)) {
        return 1;
    } else {
        return 0;
    }

}

/**
 * parses a line from the file and splits them into the three provided buffers
 * We expect each string to the inside quotes or else everything breaks
 * return values:
 * 0 : End of file
 * 1 : Successfully parsed the test
 * 2 : Unsuccessfully parsed the test
 */
int parse_line(FILE *f, char *p, char *s, char *m) {
    char str[TEST_MAX_STRING_SIZE];
    char *c = str;

    // Copying the line into the string
    while ((*c = fgetc(f)) != EOF) {
        if (*c == '\n') break;
        c++;
    }
    if (*c == EOF) return 0;

    c = str;

    if (*c != '"') { 
        if (*c != '\n')
            printf("\n%s\n", str);
        return 2;
    } else if (count_quotations(str) != 6) {
        printf("\nSkipping badly formatted test - > %s", str);
        return 2;
    }

    c = collect_string_in_quotes(c, p);
    c = collect_string_in_quotes(c, s);
    c = collect_string_in_quotes(c, m);

    return 1;
}

// Collects a string inside double quotations non-inclusively
// returns a pointer at the end of the quote
char *collect_string_in_quotes(char *c, char *strp) {
    // Chomping whitespace
    while (*c != '"') c++;

    // Moving into the string we want to collect
    c++;

    // Collecting the string
    while (*c != '"') *strp++ = *c++;

    // Null terminating the string
    *strp = '\0';

    // Moving past the quotation mark we stopped on so we can call this again
    return ++c;
}

// Counts the number of quotes in a string
int count_quotations(char *s) {
    int i = 0;
    while (*s != '\0')
        if (*s++ == '"') i++;

    return i;
}
