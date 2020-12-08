#include "SDL_plus.h"


#include "logger.h"
#include <string.h>


double GetPerformanceTime(void) {
    return ((double)SDL_GetPerformanceCounter() /
	    (double)SDL_GetPerformanceFrequency());
}


#define MAX_FILEPATH_COUNT 4
#define MAX_FILEPATH_LEN 256
static int base_filepath_index = 0;
static char base_filepaths[MAX_FILEPATH_COUNT][MAX_FILEPATH_LEN];
static int base_filepath_len;


int RememberBasePath(void) {
    char* temp = SDL_GetBasePath();

    if (!temp) {
	Err("Couldn't get the base path because %s\n", SDL_GetError());
	return SDL_ERR;
    }

    for (int i=0; i<MAX_FILEPATH_COUNT; i++) {
	strcpy(base_filepaths[i], temp);
    }
    base_filepath_len = strlen(temp);
    SDL_free(temp);
    return SDL_OK;
}


const char * FromBase(const char * filepath) {
    base_filepath_index = base_filepath_index % 4;
    base_filepaths[base_filepath_index][base_filepath_len] = '\0';
    strcat(base_filepaths[base_filepath_index], filepath);
    return base_filepaths[base_filepath_index++];
}


