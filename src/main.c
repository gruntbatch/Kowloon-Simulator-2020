#include "area.h"
#include "arguments.h"
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

#define DEFAULT_WINDOW_WIDTH 1280
#define DEFAULT_WINDOW_HEIGHT 720

#define EYE_HEIGHT 1.70

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

static union IVector2 INTERNAL_RESOLUTION;

static union IVector2 calculate_internal_resolution(union IVector2 resolution) {
    /* TODO Take fullscreen status and relative size of screen into account */
    return IVector2(resolution.x / 4, resolution.y / 4);
}

static float internal_aspect_ratio(void) {
    return (float) INTERNAL_RESOLUTION.x / (float) INTERNAL_RESOLUTION.y;
}

static struct Framebuffer internal_framebuffer;
static GLuint internal_framebuffer_program;

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

    return UP;
}

#define MOUSE_SPEED_X 5.0f
#define MOUSE_SPEED_Y 5.0f
#define MOVEMENT_SPEED 5.0f
static Agent player;
static union Matrix4 player_view;

static void walkabout(double dt) {
    static float pitch = -90.0f;
    static float yaw = 0.0f;

    union Vector2 look = GetLook();
    pitch += look.y * MOUSE_SPEED_Y * dt;
    pitch = clampf(-160.0f, pitch, -20.f);

    yaw += look.x * MOUSE_SPEED_X * dt;

    union Matrix4 yaw_matrix = Rotation(AxisAngle(Vector3(0, 0, 1), to_radians(yaw)));

    union Vector2 move = GetMove();
    union Vector2 movement = Transform4(InvertM4(yaw_matrix),
					Vector4(move.x, move.y, 0, 1)).xy;

    MoveAgent(player, movement, dt);

    player_view = MulM4(Rotation(MulQ(AxisAngle(Vector3(1, 0, 0), to_radians(pitch)),
				      AxisAngle(Vector3(0, 0, 1), to_radians(yaw)))),
			InvertM4(Translation(Add3(GetAgentPosition(player),
						  Vector3(0, 0, EYE_HEIGHT)))));
}

static char* area_to_load;

static enum Continue loop(void) {
    GLuint vertex_color_program = LoadProgram(FromBase("assets/shaders/world_space.vert"),
					      FromBase("assets/shaders/vertex_color.frag"));
    GLuint little_light_program = LoadProgram(FromBase("assets/shaders/little_light.vert"),
					      FromBase("assets/shaders/textured_vertex_color.frag"));
    GLuint atlas_texture = LoadTexture(FromBase("assets/textures/atlas.png"));

    if (!area_to_load) {
	return DOWN;
    }
    
    Navmesh navmesh = LoadNavmesh(FromBase(area_to_load));
    Agent player = CreateAgent(navmesh);
    walkabout(0);

    GLuint64 vertex_array = rtGenVertexArray();
    rtBindVertexArray(vertex_array);
    Area area = LoadArea(FromBase(area_to_load));
    rtFillBuffer();
    
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
	    walkabout(delta_time);

	    time += delta_time;
	    accumulator -= delta_time;
	}

	double alpha = accumulator / delta_time;

	/* Call update functions */
	
	/* Draw to internal framebuffer */
	{
	    glBindFramebuffer(GL_FRAMEBUFFER, internal_framebuffer.buffer);
	    glViewport(0, 0, INTERNAL_RESOLUTION.x, INTERNAL_RESOLUTION.y);
	
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	    /* imView(LookAt(Vector3(10 * sinf(current_time / 10), */
				  /* 10 * cosf(current_time / 10), */
				  /* 10), Vector3(0, 0, 0), Vector3(0, 0, 1))); */
	    /* union Vector3 position = GetAgentPosition(agent); */
	    /* position = Add3(position, Vector3(0, 0, EYE_HEIGHT)); */
	    /* imView(LookAt(position, Vector3(0, 0, 0), Vector3(0, 0, 1))); */
	    imView(player_view);
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
		imDrawAgent(player, 1.0);
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
	}
    }
    
    Rung(loop, NULL);
    return Climb();
}
