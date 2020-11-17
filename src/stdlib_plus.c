#include "stdlib_plus.h"


#include <stdio.h>


char * fopenstr(const char * filepath) {
    FILE * f = fopen(filepath, "r");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    int length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char * buffer = malloc(length + 1);
    if (buffer) {
        fread(buffer, sizeof(char), length, f);
    }
    buffer[length] = '\0';
    fclose(f);
    return buffer;
}
