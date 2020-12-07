#include "SDL_plus.h"


#include "logger.h"
#include <string.h>


double GetPerformanceTime(void) {
    return ((double)SDL_GetPerformanceCounter() /
	    (double)SDL_GetPerformanceFrequency());
}


#define MAX_FILEPATH_LEN 256
static char base_filepath[MAX_FILEPATH_LEN];
static char* base_filepath_end;


int RememberBasePath(void) {
    char* temp = SDL_GetBasePath();

    if (!temp) {
	Err("Couldn't get the base path because %s\n", SDL_GetError());
	return SDL_ERR;
    }
    
    strcpy(base_filepath, temp);
    SDL_free(temp);
    base_filepath_end = base_filepath + strlen(base_filepath);
    return SDL_OK;
}


const char * FromBase(const char * filepath) {
    *base_filepath_end = '\0';
    strcat(base_filepath, filepath);
    return base_filepath;
}


