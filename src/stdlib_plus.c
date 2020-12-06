#include "stdlib_plus.h"


#include <stdio.h>


char * fopenstr(const char * filepath) {
    /* So if I understand things right, we should open files in binary
       mode, as the size difference between `\n` and `\r\n` will
       affect how fseek reads the file; that is, on Windows, because
       it expects newlines to be 2 bytes long, it will end up seeking
       beyond the actual end of the file. */
    FILE * f = fopen(filepath, "rb");
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
    printf("FOPENSTR %s >>>\n%s\n<<< FOPENSTR\n", filepath, buffer);
    return buffer;
}
