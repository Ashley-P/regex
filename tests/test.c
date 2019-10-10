#include <stdlib.h>
#include <stdio.h>
#include "regex.h"



void main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: test.exe <PATTERN> <STRING>");
        exit(0);
    }
    
    regex(argv[1], argv[2]);
}
