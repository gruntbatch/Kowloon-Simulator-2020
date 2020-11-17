#pragma once


#include <SDL.h>


#define SDL_ERR -1
#define SDL_OK 0


double GetPerformanceTime(void);


int RememberBasePath(void);
const char * FromBase(const char * filepath);


/* int SDL_RememberPrefPath(void); */
/* const char * FromPref(const char * filepath); */
