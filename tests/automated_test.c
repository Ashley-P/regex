#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "regex.h"

#define TEST_MAX_STRING_SIZE 64

// This file incldues some automated tests
static int test_num = 1;
static char default_fp[] = "./bin/resources/automated.txt";

// returns 1 if the value is what it should be
int run_test(char *pattern, char *string, char *match) {
    printf("----- Test %d -----\n", test_num);
    printf("Pattern %s : String %s : Match %s\n", pattern, string, match);
    char *str;
    str = regex(pattern, string, REGEX_SURPRESS_LOGGING);
    printf("%s -> %s\n\n", str, match);
    test_num++;
    if (!strcmp(str, match)) {
        return 1;
    } else {
        return 0;
    }

}


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

    // We don't check whether the test files are correct so don't be wrong
    int test_num = 0;
    while (fscanf(f, "%s%s%s", pattern, string, match) != EOF) {
        assert(run_test(pattern, string, match));
        test_num++;
    }
    // If we get here then everything is complete
    printf("\n%d tests completed successfully!\n", test_num);
}
