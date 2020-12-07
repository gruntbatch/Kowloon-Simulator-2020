#include "area.h"
#include "events.h"
#include "framebuffer.h"
#include "GL_plus.h"
#include "immediate.h"
#include "logger.h"
#include "navigation.h"
#include "retained.h"
#include "SDL_plus.h"

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

    GLenum err = glewInit();
    if (GLEW_OK != err) {
      Err("Unable to initialize GLEW because %s\n", glewGetErrorString(err));
      return STOP;
    }

    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, 1280, 720);
    glClearColor(0.1, 0.1, 0.1, 1.0);

    glLogErrors();

    return GO;
}

static void delete_gl_context(void) {
    if (context) {
	SDL_GL_DeleteContext(context);
    }
}

/* TODO Pick internal resolution based on user's resolution */
static struct Framebuffer internal_framebuffer;
static GLuint internal_framebuffer_program;

static enum Continue create_renderer(void) {
    internal_framebuffer = CreateFramebuffer(320, 240);
    internal_framebuffer_program = LoadProgram(LoadShader(GL_VERTEX_SHADER,
							  FromBase("assets/shaders/world_space.vert")),
					       LoadShader(GL_FRAGMENT_SHADER,
							  FromBase("assets/shaders/textured.frag")));
    
    imInitTransformBuffer();
    imInitInternalVertexArray();

    return GO;
}

static enum Continue loop(void) {
    GLuint vertex_color_program = LoadProgram(LoadShader(GL_VERTEX_SHADER,
							 FromBase("assets/shaders/world_space.vert")),
					      LoadShader(GL_FRAGMENT_SHADER,
							 FromBase("assets/shaders/vertex_color.frag")));
    GLuint little_light_program = LoadProgram(LoadShader(GL_VERTEX_SHADER,
							 FromBase("assets/shaders/little_light.vert")),
					      LoadShader(GL_FRAGMENT_SHADER,
							 FromBase("assets/shaders/textured_vertex_color.frag")));
    GLuint atlas_texture = LoadTexture(FromBase("assets/textures/atlas.png"));

    Navmesh navmesh = LoadNavmesh(FromBase("assets/areas/alley_01"));
    Agent agent = CreateAgent(navmesh);

    GLuint64 vertex_array = rtGenVertexArray();
    rtBindVertexArray(vertex_array);
    Area area = LoadArea(FromBase("assets/areas/alley_01"));
    rtFillBuffer();

    /* Initialize matrices */
    imModel(Matrix4(1));
    imView(Matrix4(1));
    imProjection(Orthographic(0, 1280, 0, 720, -1, 1));

    double delta_time = 1.0 / 30.0; /* TODO Replace with a constant */

    double time = 0.0;
    double accumulator = 0.0;

    double current_time = GetPerformanceTime();
    double initial_time = current_time;

    while (!HasQuit()) {
	double new_time = GetPerformanceTime();
	double frame_time = new_time - current_time;
	if (frame_time > 0.25) {
	    frame_time = 0.25;
	}
	current_time = new_time;
	accumulator += frame_time;

	while (accumulator >= delta_time) {
	    /* Call fixed update functions */
	    MoveAgent(agent, GetMove(), delta_time);

	    time += delta_time;
	    accumulator -= delta_time;
	}

	double alpha = accumulator / delta_time;

	/* Call update functions */
	PollEvents();
	
	/* Draw to internal framebuffer */
	{
	    glBindFramebuffer(GL_FRAMEBUFFER, internal_framebuffer.buffer);
	    glViewport(0, 0,
		       internal_framebuffer.resolution.x,
		       internal_framebuffer.resolution.y);
	
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	    imView(LookAt(Vector3(10 * sinf(current_time / 10),
				  10 * cosf(current_time / 10),
				  10), Vector3(0, 0, 0), Vector3(0, 0, 1)));
	    /* TODO Use internal resolution to calculate aspect ratio */
	    imProjection(Perspective(90, 1280.0 / 720.0, 0.1, 100.0));

	    /* Draw the area */
	    imModel(Matrix4(1));

	    imUseProgram(little_light_program);
	    imBindTexture(GL_TEXTURE_2D, atlas_texture);

	    {
		rtBindVertexArray(vertex_array);
		DrawArea(area);
		rtFlush();
	    }
	    
	    /* Draw the navigation mesh */
	    imUseProgram(vertex_color_program);

	    {
		glDepthMask(GL_FALSE);
		imBindVertexArray();
		imDrawNavmesh(navmesh);
		imDrawAgent(agent, 1.0);
		imFlush();
		glDepthMask(GL_TRUE);
	    }
	}

	/* Draw to the window's default framebuffer */
	{
	    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	    glViewport(0, 0, 1280, 720);

	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	    imModel(Matrix4(1));
	    imView(Matrix4(1));
	    imProjection(Orthographic(0, 1, 0, 1, -1, 1));

	    imUseProgram(internal_framebuffer_program);
	    imBindTexture(GL_TEXTURE_2D, internal_framebuffer.color);

	    /* Fill the screen with a single quad */
	    imBindVertexArray();
	    imBegin(GL_TRIANGLE_STRIP); {
		imColor3ub(0, 0, 0);
		imTexCoord2f(0, 0);
		imVertex2f(0, 0);

		imColor3ub(255, 0, 0);
		imTexCoord2f(1, 0);
		imVertex2f(1, 0);

		imColor3ub(0, 255, 0);
		imTexCoord2f(0, 1);
		imVertex2f(0, 1);

		imColor3ub(255, 255, 0);
		imTexCoord2f(1, 1);
		imVertex2f(1, 1);
	    } imEnd();
	    imFlush();
	}
	
	glLogErrors();

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
