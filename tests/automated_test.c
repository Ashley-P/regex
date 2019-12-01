#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "regex.h"


// This file incldues some automated tests
static int test_num = 1;

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


int main(void) {
    assert(run_test("re", "re", "re"));
}
