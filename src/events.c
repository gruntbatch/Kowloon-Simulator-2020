#include "events.h"
#include "logger.h"


static SDL_bool has_quit = SDL_FALSE;


static union Vector2 look = { .x=0, .y=0 };
static union Vector2 move = { .x=0, .y=0 };


void PollEvents(void) {
    look = Vector2(0, 0);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
	switch (e.type) {
	case SDL_QUIT:
	    has_quit = SDL_TRUE;
	    break;
	case SDL_KEYDOWN:
	    if (e.key.repeat) {
		/* pass */
	    } else {
		switch (e.key.keysym.sym) {
		case SDLK_ESCAPE:
		    has_quit = SDL_TRUE;
		    break;
                case SDLK_w:
                    move.y += 1;
                    break;
                case SDLK_s:
                    move.y -= 1;
                    break;
                case SDLK_a:
                    move.x -= 1;
                    break;
                case SDLK_d:
                    move.x += 1;
                    break;
		default:
		    break;
		}
	    }
	    break;
        case SDL_KEYUP:
	    if (e.key.repeat) {
		/* pass */
	    } else {
		switch (e.key.keysym.sym) {
		case SDLK_w:
		    move.y -= 1;
		    break;
		case SDLK_s:
		    move.y += 1;
		    break;
		case SDLK_a:
		    move.x += 1;
		    break;
		case SDLK_d:
		    move.x -= 1;
		    break;
		default:
		    break;
		}
	    }
            break;
        case SDL_MOUSEMOTION:
            look = Add2(look, Vector2(e.motion.xrel, e.motion.yrel));
            break;
	default:
	    break;
	}
    }
}


SDL_bool HasQuit(void) {
    return has_quit;
}


union Vector2 GetLook(void) {
    return look;
}


union Vector2 GetMove(void) {
    return move;
}
