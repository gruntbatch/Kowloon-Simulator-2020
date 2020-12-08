#include "arguments.h"


#include "logger.h"
#include <string.h>
#include <stdio.h>


static int get_index(char* argv[], const char* flag) {
    for (int i=0; *argv; i++, argv++) {
	if (strcmp(flag, *argv) == 0) {
	    return i;
	}
    }
    return 0;
}


int got_flag(char* argv[], const char* flag) {
    return get_index(argv, flag) != 0;
}


int got_ints(char* argv[], const char* flag, int max_count, int out[]) {
    int index = get_index(argv, flag);
    if (index == 0) {
	return 0;
    }

    int count = 0;
    for (int i=index + 1; argv[i] && count<max_count; count++, i++) {
	int d;
	if (sscanf(argv[i], "%d", &d) != 1) {
	    break;
	}
	if (out) {
	    out[count] = d;
	}
    }

    return count;
}


int got_strings(char* argv[], const char* flag, int max_count, char* out[]) {
    int index = get_index(argv, flag);
    if (index == 0) {
	return 0;
    }

    int count = 0;
    for (int i=index + 1; argv[i] && count<max_count; count++, i++) {
	if (out) {
	    out[count] = argv[i];
	}
    }

    return count;
}
