#pragma once


#include "SDL_plus.h"


enum Continue {
    DOWN = SDL_ERR,
    UP = SDL_OK,
};


typedef enum Continue (*Up)(void);
typedef void (*Down)(void);


void Rung(Up up, Down down);
int Climb(void);
