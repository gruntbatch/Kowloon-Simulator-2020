#include "events.h"
#include "immediate.h"
#include "logger.h"
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

struct Framebuffer {
    union Vector2 resolution;
    GLuint color;
    GLuint depth;
    GLuint buffer;
};

/* TODO Pick internal resolution based on user's resolution */
static struct Framebuffer internal = { { .x=320, .y=240 } };

static GLuint draw_buffer_program;

static enum Continue create_renderer(void) {
    glGenTextures(2, &internal.color);
    glGenFramebuffers(1, &internal.buffer);

    glBindFramebuffer(GL_FRAMEBUFFER, internal.buffer); {
	glBindTexture(GL_TEXTURE_2D, internal.color); {
            glTexImage2D(GL_TEXTURE_2D,
                         0, GL_RGB,
                         internal.resolution.x, internal.resolution.y,
                         0, GL_RGB,
                         GL_UNSIGNED_BYTE,
                         NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } glBindTexture(GL_TEXTURE_2D, 0);

        glBindTexture(GL_TEXTURE_2D, internal.depth); {
            glTexImage2D(GL_TEXTURE_2D,
                         0, GL_DEPTH24_STENCIL8,
                         internal.resolution.x, internal.resolution.y,
                         0,
                         GL_DEPTH_STENCIL,
                         GL_UNSIGNED_INT_24_8,
                         NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);           
        } glBindTexture(GL_TEXTURE_2D, 0);

        glLogErrors();

        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D,
                               internal.color,
                               0);

        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_DEPTH_STENCIL_ATTACHMENT,
                               GL_TEXTURE_2D,
                               internal.depth,
                               0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER)
            != GL_FRAMEBUFFER_COMPLETE) {
            Err("Unable to complete internal framebuffer.\n");
            return STOP;
        }
    } glBindFramebuffer(GL_FRAMEBUFFER, 0);

    draw_buffer_program = LoadProgram(LoadShader(GL_VERTEX_SHADER,
						 FromBase("assets/shaders/world_space.vert")),
				      LoadShader(GL_FRAGMENT_SHADER,
						 FromBase("assets/shaders/draw_buffer.frag")));
    
    imInitTransformBuffer();
    imInitInternalVertexArray();

    return GO;
}

struct Transition {
    int target;
    union Vector2 position;
    union Vector2 size;
    union Matrix4 transform;
};

struct Triangle {
    union {
	struct {
	    union Vector2 p0;
	    union Vector2 p1;
	    union Vector2 p2;
	};
	union Vector2 p[3];
    };
    union {
	struct {
	    int n0; /* 0, 1 */
	    int n1; /* 1, 2 */
	    int n2; /* 2, 0 */
	};
	int n[3];
    };
    union {
	struct {
	    int t0;
	    int t1;
	    int t2;
	};
	int t[3];
    };
};

struct Transition trans[2] = {
    { .target=1,
      .position={ .x=0, .y=-1 },
      .size={ .x=1.5, .y=0.5 } },
    { .target=0,
      .position={ .x=0, .y=1 },
      .size={ .x=1.5, .y=0.5 } },
};
struct Triangle navmesh[4] = {
    { .p0={ .x=0, .y=-1 }, .p1={ .x=-1, .y=-1 }, .p2={ .x=-1, .y=0 },
      .n0=3, .n1=-1, .n2=1,
      .t0=1, .t1=-1, .t2=-1, },
    { .p0={ .x=0, .y=-1 }, .p1={ .x=-1, .y=0 }, .p2={ .x=0, .y=0 },
      .n0=0, .n1=2, .n2=-1,
      .t0=-1, .t1=-1, .t2=-1, },
    { .p0={ .x=-1, .y=0 }, .p1={ .x=0, .y=1 }, .p2={ .x=0, .y=0 },
      .n0=3, .n1=-1, .n2=1,
      .t0=-1, .t1=-1, .t2=-1, },
    { .p0={ .x=-1, .y=0 }, .p1={ .x=-1, .y=1 }, .p2={ .x=0, .y=1 },
      .n0=-1, .n1=0, .n2=2,
      .t0=-1, .t1=0, .t2=-1, },
 };

static int tridex = 0;

static union Vector2 velocity = { 0 };
static union Vector2 position = { .x=0, .y=0 };

static int inside = 0;

static float sign(union Vector2 q, union Vector2 a, union Vector2 b)
{
    /* return (q.x - b.x) * (a.y - b.y) - (a.x - b.x) * (q.y - b.y); */
    return (q.x - a.x) * (b.y - a.y) - (b.x - a.x) * (q.y - a.y);
}

// TODO Replace with an actual barycentric check lol
static int bary_check(struct Triangle tri, union Vector2 pos) {
    float d0, d1, d2;
    d0 = sign(pos, tri.p0, tri.p1);
    d1 = sign(pos, tri.p1, tri.p2);
    d2 = sign(pos, tri.p2, tri.p0);

    int posi, negi;
    negi = (d0 < 0) || (d1 < 0) || (d2 < 0);
    posi = (d0 > 0) || (d1 > 0) || (d2 > 0);

    return !(negi && posi);
}

static int transition(struct Triangle tri, union Vector2 pos) {
    float d0, d1, d2;
    d0 = sign(pos, tri.p0, tri.p1);
    d1 = sign(pos, tri.p1, tri.p2);
    d2 = sign(pos, tri.p2, tri.p0);

    if (d0 < 0) {
	return tri.n0;
    } else if (d1 < 0) {
	return tri.n1;
    } else if (d2 < 0) {
	return tri.n2;
    }
    
    return -1;
}



static float distance_point_to_line(union Vector2 p, union Vector2 a, union Vector2 b, union Vector2* out) {
    /* Adapted from https://stackoverflow.com/a/1501725 */
    // Return minimum distance between line segment ab and point p
    union Vector2 nearest;
    float dist;

    float mag2 = DistanceSquared2(a, b); // i.e. |w-v|^2 -  avoid a sqrt
    if (mag2 == 0.0) {
	nearest = a;
	dist = Distance2(p, a);
    } else {
	// Consider the line extending the segment, parameterized as v + t (w - v).
	// We find projection of point p onto the line. 
	// It falls where t = [(p-v) . (w-v)] / |w-v|^2
	// We clamp t from [0,1] to handle points outside the segment vw.
	float t = fmaxf(0, fminf(1, Dot2(Sub2(position, a), Sub2(b, a)) / mag2));
	nearest = Add2(a, Scale2(Sub2(b, a), t)); // Nearest falls on the segment
	dist = Distance2(position, nearest);
    }

    if (out) {
	*out = nearest;
    }

    return dist;
}



static void move() {
    struct Triangle tri = navmesh[tridex];
    
    float d0, d1, d2;
    d0 = sign(position, tri.p0, tri.p1);
    d1 = sign(position, tri.p1, tri.p2);
    d2 = sign(position, tri.p2, tri.p0);

    union Vector2 a, b;
    int edge = -1;
    int next = -1;
    int tran = -1;

    if (d0 < 0) {
	a = tri.p0; b = tri.p1;
	edge = 0;
	next = tri.n0;
	tran = tri.t0;
    } else if (d1 < 0) {
	a = tri.p1; b = tri.p2;
	edge = 1;
	next = tri.n1;
	tran = tri.t1;
    } else if (d2 < 0) {
	a = tri.p2; b = tri.p0;
	edge = 2;
	next = tri.n2;
	tran = tri.t2;
    } else {
	/* return; */
	a = tri.p0; b = tri.p1;
    }

    union Vector2 nearest;
    float dist = distance_point_to_line(position, a, b, &nearest);

    imColor3ub(255, 255, 255);
    imBegin(GL_LINES); {
	imVertex2(nearest);
	imVertex2(position);
    } imEnd();

    if (next < 0) {
	
    } else {
	/* printf("TRIDEX %d\n", next); */
	if (tran < 0) {

	} else {
	    struct Transition t = trans[tran];
	    struct Transition target = trans[t.target];
	    union Vector2 offset = Sub2(t.position, target.position);
	    position = Add2(position, offset);
	}
	tridex = next;
    }
}

static enum Continue loop(void) {
    GLuint program = LoadProgram(LoadShader(GL_VERTEX_SHADER,
					    FromBase("assets/shaders/world_space.vert")),
				 LoadShader(GL_FRAGMENT_SHADER,
					    FromBase("assets/shaders/vertex_color.frag")));

    /* GLuint64 vertex_array = rtGenVertexArray(); */
    /* rtBindVertexArray(vertex_array); */
    /* GLuint64 mesh_id = rtLoadMesh(FromBase("assets/models/Floor13.anio")); */
    /* rtFillBuffer(); */

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
	    /* TODO Call fixed update functions */

	    time += delta_time;
	    accumulator -= delta_time;
	}

	double alpha = accumulator / delta_time;

	/* Call update functions */
	PollEvents();
	
	/* Draw to internal framebuffer */
	glBindFramebuffer(GL_FRAMEBUFFER, internal.buffer);
	glViewport(0, 0, internal.resolution.x, internal.resolution.y);
	    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	imModel(Matrix4(1));
	imView(Matrix4(1));
	imProjection(Orthographic(0, internal.resolution.x,
				  0, internal.resolution.y,
				  -1, 1));

	imUseProgram(program);

	imBindVertexArray();
	imBegin(GL_TRIANGLES); {
	    imColor3ub(25, 0, 0);
	    imVertex2f(0, 0);

	    imColor3ub(0, 25, 0);
	    imVertex2f(internal.resolution.x / 2.0, internal.resolution.y);

	    imColor3ub(0, 0, 25);
	    imVertex2f(internal.resolution.x, 0);
	} imEnd();
	imFlush();

	glClear(GL_DEPTH_BUFFER_BIT);

	imModel(Matrix4(1));
	imView(LookAt(Vector3(-1, -2, 2), Vector3(0, 0, 0), Vector3(0, 0, 1)));
	/* TODO Use internal resolution to calculate aspect ratio */
	imProjection(Perspective(90, 1280.0 / 720.0, 0.1, 100.0));

	/* rtBindVertexArray(vertex_array); */
	/* rtDrawArrays(GL_TRIANGLES, mesh_id); */
	/* rtFlush(); */
	imBindVertexArray();
	imColor3ub(12, 100, 45);
	for (int i=0; i<2; ++i) {
	    struct Transition t = trans[i];
	    imBegin(GL_LINE_LOOP); {
		imVertex2f(t.position.x - t.size.x,
			   t.position.y - t.size.y);
		imVertex2f(t.position.x - t.size.x,
			   t.position.y + t.size.y);
		imVertex2f(t.position.x + t.size.x,
			   t.position.y + t.size.y);
		imVertex2f(t.position.x + t.size.x,
			   t.position.y - t.size.y);
	    } imEnd();
	}
	
	imColor3ub(100, 50, 0);
	for (int i=0; i<4; ++i) {
	    struct Triangle t = navmesh[i];
	    imBegin(GL_LINE_LOOP); {
		imVertex2(t.p0);
		imVertex2(t.p1);
		imVertex2(t.p2);
	    } imEnd();
	}
	imFlush();
	glClear(GL_DEPTH_BUFFER_BIT);
	
	if (tridex != -1) {
	    imColor3ub(255, 255, 0);
	    struct Triangle t = navmesh[tridex];
	    imBegin(GL_LINE_LOOP); {
		imVertex2(t.p0);
		imVertex2(t.p1);
		imVertex2(t.p2);
	    } imEnd();
	}

	/* Move point */
	velocity = Add2(velocity, GetMove());
	if (Magnitude2(velocity) >= 1.0) {
	    velocity = Scale2(Normalize2(velocity), 1.0);
	}
	position = Add2(Scale2(velocity, delta_time), position);
	velocity = Scale2(velocity, 0.7);

	move();
	/* int t = transition(navmesh[tridex], position); */

	/* if (t != -1) { */
	/*     tridex = t; */
	/* } */
	

	inside = bary_check(navmesh[tridex], position);

	if (inside) {
	    imColor3ub(0, 255, 255);
	} else {
	    imColor3ub(255, 127, 0);
	}
	imModel(Translation(Vector3(position.x, position.y, 0)));
	imBegin(GL_LINE_LOOP); {
	    imVertex2f(0, 0.1);
	    imVertex2f(0.1, 0);
	    imVertex2f(0, -0.1);
	    imVertex2f(-0.1, 0);
	} imEnd();
	imFlush();

	/* Draw to default framebuffer */
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, 1280, 720);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	imModel(Matrix4(1));
	imView(Matrix4(1));
	imProjection(Orthographic(0, 1, 0, 1, -1, 1));

	imUseProgram(draw_buffer_program);
	imBindTexture(GL_TEXTURE_2D, internal.color);

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

	    imColor3ub(0, 0, 255);
	    imTexCoord2f(1, 1);
	    imVertex2f(1, 1);
	} imEnd();
	imFlush();
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
