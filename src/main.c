#include "immediate.h"
#include "logger.h"
#include "SDL_plus.h"
#include "SDL_opengl.h"

enum Continue {
    STOP,
    GO,
};

static enum Continue log_verbosely(void) {
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

    return GO;
}

static enum Continue remember_filepaths(void) {
    if (RememberBasePath() != SDL_OK) {
	Err("Unable to remember the base path because %s\n", SDL_GetError());
	return STOP;
    }

    return GO;
}

static enum Continue init_sdl(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != SDL_OK) {
	Err("Unable to initialize SDL because %s\n", SDL_GetError());
	return STOP;
    }
    
    return GO;
}

static void quit_sdl(void) {
    SDL_Quit();
}

static enum Continue set_gl_attributes(void) {
    /* Specify OpenGL version */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
			SDL_GL_CONTEXT_PROFILE_CORE);

    /* Set buffer attributes */
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    return GO;
}

static SDL_Window* window;

static enum Continue open_window(void) {
    window = SDL_CreateWindow("a.out",
			      SDL_WINDOWPOS_CENTERED,
			      SDL_WINDOWPOS_CENTERED,
			      1280,
			      720,
			      SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

    if (!window) {
	Err("Unable to open a window because %s\n", SDL_GetError());
	return STOP;
    }

    /* SDL_SetRelativeMouseMode(SDL_TRUE); */

    return GO;
}

static void close_window(void) {
    if (window) {
	SDL_DestroyWindow(window);
    }
}

static SDL_GLContext context;

static enum Continue create_gl_context(void) {
    context = SDL_GL_CreateContext(window);

    if (!context) {
	Err("Unable to create an OpenGL context because %s\n",
	    SDL_GetError());
	return STOP;
    }

    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, 1280, 720);
    glClearColor(1.0, 0.0, 0.0, 1.0);

    return GO;
}

static void delete_gl_context(void) {
    if (context) {
	SDL_GL_DeleteContext(context);
    }
}

static enum Continue create_renderer(void) {
    imInitTransformBuffer();
    imInitInternalVertexArray();

    return GO;
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

static enum Continue loop(void) {
    GLuint program = LoadProgram(LoadShader(GL_VERTEX_SHADER,
					    FromBase("assets/shaders/world_space.vert")),
				 LoadShader(GL_FRAGMENT_SHADER,
					    FromBase("assets/shaders/vertex_color.frag")));

    /* Initialize matrices */
    imModel(Matrix4(1));
    imView(Matrix4(1));
    imProjection(Orthographic(0, 1280, 0, 720, -1, 1));

    double delta_time = 1.0 / 30.0; /* TODO Replace with a constant */

    double time = 0.0;
    double accumulator = 0.0;

    double current_time = GetPerformanceTime();
    double initial_time = current_time;

    RUN = SDL_TRUE;
    while (RUN) {
	double new_time = GetPerformanceTime();
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

	imBindVertexArray();
	imUseProgram(program);
	imBegin(GL_TRIANGLES); {
	    imColor3ub(255, 0, 0);
	    imVertex2f(0, 0);

	    imColor3ub(0, 255, 0);
	    imVertex2f(640, 720);

	    imColor3ub(0, 0, 255);
	    imVertex2f(1280, 0);
	} imEnd();
	imFlush();

	SDL_GL_SwapWindow(window);
    }
    
    return STOP;
}   

typedef enum Continue (*Up)(void);
typedef void (*Down)(void);

struct Rung {
    Up up;
    Down down;
};

int main(int argc, char* argv[]) {
    struct Rung ladder[] = {{ .up=log_verbosely },
			    { .up=remember_filepaths },
			    { .up=init_sdl, .down=quit_sdl },
			    { .up=set_gl_attributes },
			    { .up=open_window, .down=close_window },
			    { .up=create_gl_context, .down=delete_gl_context },
			    { .up=create_renderer },
			    { .up=loop }};
    struct Rung* rung = ladder;

    for (;;) {
	if (rung->up && rung->up() == GO) {
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

    return ErrorCount();
}
