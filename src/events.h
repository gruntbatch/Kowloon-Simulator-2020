#pragma once


#include "mathematics.h"
#include "SDL_plus.h"


void PollEvents(void);


SDL_bool HasQuit(void);
union Vector2 GetLook(void);
union Vector2 GetMove(void);
