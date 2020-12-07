#include "area.h"
#include "events.h"
#include "framebuffer.h"
#include "GL_plus.h"
#include "immediate.h"
#include "ladder.h"
#include "logger.h"
#include "navigation.h"
#include "retained.h"
#include "SDL_plus.h"

#define TITLE "Cyberpunk1997"

#define MAX_INTERNAL_WIDTH 480
#define MAX_INTERNAL_HEIGHT 270

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

static union IVector2 resolution = { .x=1280, .y=720 };

static float aspect_ratio(void) {
    return (float) resolution.x / (float) resolution.y;
}

static SDL_Window* window;

static enum Continue open_window(void) {
    window = SDL_CreateWindow(TITLE,
			      SDL_WINDOWPOS_CENTERED,
			      SDL_WINDOWPOS_CENTERED,
			      resolution.x,
			      resolution.y,
			      SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

    if (!window) {
	Err("Unable to open a window because %s\n", SDL_GetError());
	return DOWN;
    }

    /* SDL_SetRelativeMouseMode(SDL_TRUE); */

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

    glViewport(0, 0, resolution.x, resolution.y);
    glClearColor(0.1, 0.1, 0.1, 1.0);

    glLogErrors();

    return UP;
}

static void delete_gl_context(void) {
    if (context) {
	SDL_GL_DeleteContext(context);
    }
}

static union IVector2 internal_resolution;

static union IVector2 calculate_internal_resolution(union IVector2 resolution) {
    /* TODO Take fullscreen status and relative size of screen into account */
    return IVector2(resolution.x / 4, resolution.y / 4);
}

static float internal_aspect_ratio(void) {
    return (float) internal_resolution.x / (float) internal_resolution.y;
}

static struct Framebuffer internal_framebuffer;
static GLuint internal_framebuffer_program;

static enum Continue create_renderer(void) {
    internal_resolution = calculate_internal_resolution(resolution);
    Log("For a resolution of %d by %d, the internal resolution is %d by %d\n",
	resolution.x, resolution.y, internal_resolution.x, internal_resolution.y);

    /* The internal framebuffer will never need to be bigger than 480
       by 270, so we can create a framebuffer of that size and
       modulate the viewport size */
    internal_framebuffer = CreateFramebuffer(MAX_INTERNAL_WIDTH, MAX_INTERNAL_HEIGHT);
    internal_framebuffer_program = LoadProgram(LoadShader(GL_VERTEX_SHADER,
							  FromBase("assets/shaders/world_space.vert")),
					       LoadShader(GL_FRAGMENT_SHADER,
							  FromBase("assets/shaders/textured.frag")));
    
    imInitTransformBuffer();
    imInitInternalVertexArray();

    return UP;
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
    imProjection(Orthographic(0, resolution.x, 0, resolution.y, -1, 1));

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
	    glViewport(0, 0, internal_resolution.x, internal_resolution.y);
	
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	    imView(LookAt(Vector3(10 * sinf(current_time / 10),
				  10 * cosf(current_time / 10),
				  10), Vector3(0, 0, 0), Vector3(0, 0, 1)));
	    imProjection(Perspective(90, internal_aspect_ratio(), 0.1, 100.0));

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
	    glViewport(0, 0, resolution.x, resolution.y);

	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	    imModel(Matrix4(1));
	    imView(Matrix4(1));
	    imProjection(Orthographic(0, internal_resolution.x,
				      0, internal_resolution.y,
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

int main(int argc, char* argv[]) {
    LogVerbosely();
    Rung(RememberBasePath, NULL);
    Rung(init_sdl, quit_sdl);
    Rung(set_gl_attributes, NULL);
    Rung(open_window, close_window);
    Rung(create_gl_context, delete_gl_context);
    Rung(create_renderer, NULL);
    Rung(loop, NULL);
    return Climb();
}
