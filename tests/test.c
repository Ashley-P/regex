#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "regex.h"

#define TEST_MAX_STRING_SIZE 1000

int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        printf("Usage: test.exe <PATTERN> <STRING> [OPTIONS]");
        exit(0);
    }

    char pattern[TEST_MAX_STRING_SIZE];
    char match[TEST_MAX_STRING_SIZE];
    char *returned_str;    

    memcpy(pattern, argv[1], strlen(argv[1]) + 1);
    memcpy(match, argv[2], strlen(argv[2]) + 1);

    LARGE_INTEGER startt, endt, elapsedt, freq;
    QueryPerformanceFrequency(&freq); 
    QueryPerformanceCounter(&startt);

    if (argc == 3)
        returned_str = regex(pattern, match, 0);
    else
        returned_str = regex(pattern, match, atoi(argv[3]));

    QueryPerformanceCounter(&endt);
    elapsedt.QuadPart = endt.QuadPart - startt.QuadPart;

    // Elapsed in milliseconds
    elapsedt.QuadPart *= 1000000;
    elapsedt.QuadPart /= freq.QuadPart;

    printf("%s\n", returned_str);
    printf("Completed in %li microseconds", (long) elapsedt.QuadPart);
}
