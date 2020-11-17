#include <stdio.h>
#include "SDL_plus.h"
#include "SDL_opengl.h"

static int init_sdl(void) {
    return SDL_Init(SDL_INIT_VIDEO);
}

static void quit_sdl(void) {
    SDL_Quit();
}

static int set_gl_attributes(void) {
    return 0;
}

static SDL_Window* window;

static int open_window(void) {
    window = SDL_CreateWindow("a.out",
			      SDL_WINDOWPOS_CENTERED,
			      SDL_WINDOWPOS_CENTERED,
			      1280,
			      720,
			      SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

    if (!window) {
	return 1;
    }

    /* SDL_SetRelativeMouseMode(SDL_TRUE); */

    return 0;
}

static void close_window(void) {
    if (window) {
	SDL_DestroyWindow(window);
    }
}

static SDL_GLContext context;

static int create_gl_context(void) {
    context = SDL_GL_CreateContext(window);

    if (!context) {
	return 1;
    }

    glViewport(0, 0, 1280, 720);
    glClearColor(1.0, 0.0, 0.0, 1.0);

    return 0;
}

static void delete_gl_context(void) {
    if (context) {
	SDL_GL_DeleteContext(context);
    }
}

static SDL_bool RUN;

static void poll_events(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
	switch (e.type) {
	case SDL_QUIT:
	    RUN = SDL_FALSE;
	    break;
	case SDL_KEYDOWN:
	    if (e.key.repeat) {
		/* pass */
	    } else {
		switch (e.key.keysym.sym) {
		case SDLK_ESCAPE:
		    RUN = SDL_FALSE;
		    break;
		default:
		    break;
		}
	    }
	default:
	    break;
	}
    }
}

static int loop(void) {
    double delta_time = 1.0 / 30.0; /* TODO Replace with a constant */

    double time = 0.0;
    double accumulator = 0.0;

    double current_time = SDL_GetPerformanceTime();
    double initial_time = current_time;

    RUN = SDL_TRUE;
    while (RUN) {
	double new_time = SDL_GetPerformanceTime();
	double frame_time = new_time - current_time;
	if (frame_time > 0.25) {
	    frame_time = 0.25;
	}
	current_time = new_time;
	accumulator += frame_time;

	while (accumulator >= delta_time) {
	    /* TODO Call fixed update functions */

	    time += delta_time;
	    accumulator -= delta_time;
	}

	double alpha = accumulator / delta_time;

	/* Call update functions */
	poll_events();
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapWindow(window);
    }
    
    return 1;
}

typedef int (*Up)(void);
typedef void (*Down)(void);

struct Rung {
    Up up;
    Down down;
};

int main(int argc, char* argv[]) {
    struct Rung ladder[] = {{ .up=init_sdl, .down=quit_sdl },
			    { .up=set_gl_attributes },
			    { .up=open_window, .down=close_window },
			    { .up=create_gl_context, .down=delete_gl_context },
			    { .up=loop }};
    struct Rung* rung = ladder;

    for (;;) {
	if (rung->up && rung->up() == 0) {
	    rung++;
	} else {
	    break;
	}
    }

    for (; rung >= ladder;) {
	if (rung->down) {
	    rung->down();
	}
	rung--;
    }

    return 0;
}
