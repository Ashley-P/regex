#include <assert.h>
#include <stdio.h>
#include "regex.h"

#define TEST_SHOULD_SUCCEED 1
#define TEST_SHOULD_FAIL    2

// This file incldues some automated tests

// returns 1 if the value is what it should be
int run_test(char *pattern, char *string, int outcome) {
    printf("Pattern %s : String %s : Pass or fail %s\n", pattern, string,
            (outcome == 1) ? "TEST_SHOULD_SUCCEED" : "TEST_SHOULD_FAIL");
    char *str;
    str = regex(pattern, string, REGEX_SURPRESS_LOGGING);
    if (outcome == TEST_SHOULD_SUCCEED) {
        if (*str != '\0') {
            printf("Test successful, returned string -> %s", str);
            return 1;
        } else {
            printf("Test failed");
            return 0;
        }

    } else {
        if (*str == '\0') {
            printf("Test failed (success)");
            return 1;
        } else {
            printf("Test successful (failed), returned string -> %s", str);
            return 0;
        }
    }

}


int main(void) {
    assert(run_test("re", "re", TEST_SHOULD_SUCCEED));
}
