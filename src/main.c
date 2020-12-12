#include "area.h"
#include "arguments.h"
#include "events.h"
#include "framebuffer.h"
#include "GL_plus.h"
#include "immediate.h"
#include "ladder.h"
#include "logger.h"
/* #include "navigation.h" */
#include "player.h"
#include "retained.h"
#include "SDL_plus.h"
#include "stdlib_plus.h"

#define TITLE "Cyberpunk1997"

/* Resolution stuff */
#define TARGET_INTERNAL_HEIGHT 240

#define MAX_INTERNAL_WIDTH 480
#define MAX_INTERNAL_HEIGHT 270

#define DEFAULT_WINDOW_WIDTH 1280
#define DEFAULT_WINDOW_HEIGHT 720

static enum Continue init_sdl(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != SDL_OK) {
	Err("Unable to initialize SDL because %s\n", SDL_GetError());
	return DOWN;
    }
    
    return UP;
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

    return UP;
}

static int FULLSCREEN = 0;
static union IVector2 RESOLUTION = { .x=DEFAULT_WINDOW_WIDTH,
                                     .y=DEFAULT_WINDOW_HEIGHT };

static float aspect_ratio(void) {
    return (float) RESOLUTION.x / (float) RESOLUTION.y;
}

static SDL_Window* window = NULL;

static enum Continue open_window(void) {
    window = SDL_CreateWindow(TITLE,
			      SDL_WINDOWPOS_CENTERED,
			      SDL_WINDOWPOS_CENTERED,
			      RESOLUTION.x,
			      RESOLUTION.y,
			      SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | FULLSCREEN);

    /* TODO This might not work well on high-DPI displays */
    SDL_GetWindowSize(window, &RESOLUTION.x, &RESOLUTION.y);
    Log("The window has a resolution of %d by %d\n", RESOLUTION.x, RESOLUTION.y);

    if (!window) {
	Err("Unable to open a window because %s\n", SDL_GetError());
	return DOWN;
    }

    SDL_SetRelativeMouseMode(SDL_TRUE);

    return UP;
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
	return DOWN;
    }

    GLenum err = glewInit();
    if (GLEW_OK != err) {
      Err("Unable to initialize GLEW because %s\n", glewGetErrorString(err));
      return DOWN;
    }

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.1, 0.1, 0.1, 1.0);

    glLogErrors();

    return UP;
}

static void delete_gl_context(void) {
    if (context) {
	SDL_GL_DeleteContext(context);
    }
}

static union IVector2 calculate_internal_resolution(union IVector2 resolution) {
    float divisor = (float)resolution.y / (float)TARGET_INTERNAL_HEIGHT;
    return IVector2((float)resolution.x / divisor, (float)resolution.y / divisor);
}

static union IVector2 INTERNAL_RESOLUTION;

static float internal_aspect_ratio(void) {
    return (float) INTERNAL_RESOLUTION.x / (float) INTERNAL_RESOLUTION.y;
}

static struct Framebuffer internal_framebuffer;
static GLuint internal_framebuffer_program;
static GLuint64 scenery_vertex_array;

static enum Continue create_renderer(void) {
    INTERNAL_RESOLUTION = calculate_internal_resolution(RESOLUTION);
    Log("For a resolution of %d by %d, the internal resolution is %d by %d\n",
	RESOLUTION.x, RESOLUTION.y, INTERNAL_RESOLUTION.x, INTERNAL_RESOLUTION.y);

    /* The internal framebuffer will never need to be bigger than 480
       by 270, so we can create a framebuffer of that size and
       modulate the viewport size */
    internal_framebuffer = CreateFramebuffer(MAX_INTERNAL_WIDTH, MAX_INTERNAL_HEIGHT);
    internal_framebuffer_program = LoadProgram(FromBase("assets/shaders/world_space.vert"),
					       FromBase("assets/shaders/textured.frag"));
    
    imInitTransformBuffer();
    imInitInternalVertexArray();

    scenery_vertex_array = rtGenVertexArray();

    return UP;
}

static char* area_to_load;

static enum Continue load_area(void) {
    if (!area_to_load) {
	return DOWN;
    }

    rtBindVertexArray(scenery_vertex_array);
    Area area = LoadArea(area_to_load);
    rtFillBuffer();

    InstanceArea(area);
    LinkInstancedNetworks();

    return UP;
}

static enum Continue load_areas_from_index(void) {
    char * source = fopenstr(FromBase("assets/area.index"));
    if (!source) {
	Warn("Unable to open area index. Does it exist?\n");
	return DOWN;
    }

    /* TODO Warn us if we're going over out vertex array bounds */
    rtBindVertexArray(scenery_vertex_array);

    char * line = source;
    while (line) {
	char * endline = strchr(line, '\n');
	if (endline) {
	    *endline = '\0';

	    char filepath[96];

	    int s = sscanf(line, "%s", filepath);
	    
	    if (s == 1) {
		LoadArea(filepath);
	    }
	    
	    line = endline + 1;
	} else {
	    line = NULL;
	}
    }
    
    free(source);

    rtFillBuffer();

    InstanceAreas(8);
    LinkInstancedNetworks();

    return UP;
}

static enum Continue loop(void) {
    GLuint vertex_color_program = LoadProgram(FromBase("assets/shaders/world_space.vert"),
					      FromBase("assets/shaders/vertex_color.frag"));
    GLuint little_light_program = LoadProgram(FromBase("assets/shaders/multi_lights.vert"),
					      FromBase("assets/shaders/textured_vertex_color.frag"));
    GLuint atlas_texture = LoadTexture(FromBase("assets/textures/atlas.png"));

    SpawnPlayer(GetAreaInstance(0));
    
    /* Initialize matrices */
    imModel(Matrix4(1));
    imView(Matrix4(1));
    imProjection(Orthographic(0, RESOLUTION.x, 0, RESOLUTION.y, -1, 1));

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
	    PollEvents();
	    /* walkabout(delta_time); */
	    PlayerWalkabout(delta_time);

	    time += delta_time;
	    accumulator -= delta_time;
	}

	double alpha = accumulator / delta_time;

	/* Call update functions */
	Area area = GetPlayerArea();
	
	/* Draw to internal framebuffer */
	{
	    glBindFramebuffer(GL_FRAMEBUFFER, internal_framebuffer.buffer);
	    glViewport(0, 0, INTERNAL_RESOLUTION.x, INTERNAL_RESOLUTION.y);
	
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	    imProjection(Perspective(90, internal_aspect_ratio(), 0.1, 100.0));

	    /* Draw the area */
	    imModel(Matrix4(1));

	    imUseProgram(little_light_program);
	    imBindTexture(GL_TEXTURE_2D, atlas_texture);

	    {
		rtBindVertexArray(scenery_vertex_array);
		DrawSceneryRecursively(area, -1, GetPlayerView(), 2);
		rtFlush();
	    }
	    
	    /* Draw the navigation mesh */
	    imUseProgram(vertex_color_program);

	    {
		glDepthMask(GL_FALSE);
		imBindVertexArray();
		DrawNavmesh(area);
		DrawNetwork(area);
		DrawPlayer(1.0);
		imFlush();
		glDepthMask(GL_TRUE);
	    }
	}

	/* Draw to the window's default framebuffer */
	{
	    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	    glViewport(0, 0, RESOLUTION.x, RESOLUTION.y);

	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	    imModel(Matrix4(1));
	    imView(Matrix4(1));
	    imProjection(Orthographic(0, INTERNAL_RESOLUTION.x,
				      0, INTERNAL_RESOLUTION.y,
				      -1, 1));

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
		imVertex2f(internal_framebuffer.resolution.x, 0);

		imColor3ub(0, 255, 0);
		imTexCoord2f(0, 1);
		imVertex2f(0, internal_framebuffer.resolution.y);

		imColor3ub(255, 255, 0);
		imTexCoord2f(1, 1);
		imVertex2f(internal_framebuffer.resolution.x,
			   internal_framebuffer.resolution.y);
	    } imEnd();
	    imFlush();
	}
	
	glLogErrors();

	SDL_GL_SwapWindow(window);
    }
    
    return UP;
}   

int main(int rgc, char* argv[]) {
    {
	if (got_flag(argv, "--version") == 1) {
	    printf("TODO VERSION\n");
	    return 0;
	}
	if (got_flag(argv, "--help") == 1) {
	    printf("TODO HELP\n");
	    return 0;
	}
    }
    
    LogVerbosely();
    Rung(RememberBasePath, NULL);
    Rung(init_sdl, quit_sdl);
    Rung(set_gl_attributes, NULL);
    Rung(open_window, close_window);

    {
	union IVector2 maybe_resolution;
	if (got_ints(argv, "--resolution", 2, &maybe_resolution.x) == 2) {
	    RESOLUTION = maybe_resolution;
	}

	if (got_flag(argv, "--fullscreen") == 1) {
	    FULLSCREEN = SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
    }
    
    Rung(create_gl_context, delete_gl_context);
    Rung(create_renderer, NULL);

    {
	if (got_strings(argv, "--area", 1, &area_to_load) == 1) {
	    Rung(load_area, NULL);
	} else {
	    Rung(load_areas_from_index, NULL);
	}
    }
    
    Rung(loop, NULL);
    return Climb();
}
