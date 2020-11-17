#include "SDL_plus.h"


double SDL_GetPerformanceTime(void) {
    return ((double)SDL_GetPerformanceCounter() /
	    (double)SDL_GetPerformanceFrequency());
}
