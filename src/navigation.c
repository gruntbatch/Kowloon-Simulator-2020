#include "navigation.h"


#include "immediate.h"
#include "logger.h"
#include "stdlib.h"


struct Triangle {
    union {
	struct { union Vector2 p0, p1, p2; };
	union Vector2 p[3];
    };
    union {
	struct { int n0, n1, n2; };
	int n[3];
    };
    union {
	struct { int t0, t1, t2; };
	int t[3];
    };
};


struct Transition {
    int target;
    union Vector2 position;
    union Vector2 size;
};


static union Vector2 to_barycentric(union Vector2 p, struct Triangle t) {
    union Vector2 ab, ac, ap;
    ab = Sub2(t.p1, t.p0);
    ac = Sub2(t.p2, t.p0);
    ap = Sub2(p, t.p0);

    float d00 = Dot2(ab, ab);
    float d01 = Dot2(ab, ac);
    float d11 = Dot2(ac, ac);
    float d20 = Dot2(ap, ab);
    float d21 = Dot2(ap, ac);

    float d = 1.0 / (d00 * d11 - d01 * d01);

    float u = (d11 * d20 - d01 * d21) * d;
    float v = (d00 * d21 - d01 * d20) * d;
    /* float w = 1.0f - u - v; */

    return Vector2(u, v);
}

static union Vector2 local_to_barycentric(union Vector2 p, struct Triangle t) {
    return to_barycentric(Add2(p, t.p0), t);
}

static union Vector2 from_barycentric(union Vector2 p, struct Triangle t) {
    union Vector2 ab, ac;
    ab = Sub2(t.p1, t.p0);
    ac = Sub2(t.p2, t.p0);
    return Add2(Add2(Scale2(ab, p.u), Scale2(ac, p.v)), t.p0);
}

static float sign(union Vector2 p, union Vector2 a, union Vector2 b)
{
     return (p.x - b.x) * (a.y - b.y) - (a.x - b.x) * (p.y - b.y);
}


static float distance_to_line(union Vector2 p, union Vector2 a, union Vector2 b, union Vector2* out) {
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
	float t = fmaxf(0, fminf(1, Dot2(Sub2(p, a), Sub2(b, a)) / mag2));
	nearest = Add2(a, Scale2(Sub2(b, a), t)); // Nearest falls on the segment
	dist = Distance2(p, nearest);
    }

    if (out) {
	*out = nearest;
    }

    return dist;
}


static float signed_distance_to_line(union Vector2 p, union Vector2 a, union Vector2 b, union Vector2* out) {
    return distance_to_line(p, a, b, out) * sign(p, a, b);
}


static int inside_triangle(union Vector2 pos, struct Triangle tri) {
    union Vector2 uv = to_barycentric(pos, tri);
    return (0 <= uv.u && 0 <= uv.v && uv.u + uv.v <= 1);
}


static int inside_barycentric_triangle(union Vector2 uv) {
    return (0 <= uv.u && 0 <= uv.v && uv.u + uv.v <= 1);
}


#define MAX_TRIANGLE_COUNT 64
#define MAX_TRANSITION_COUNT 8


struct Navmesh {
    int triangle_count;
    struct Triangle triangles[MAX_TRIANGLE_COUNT];
    int transition_count;
    struct Transition transitions[MAX_TRANSITION_COUNT];
};


#define MAX_NAVMESH_COUNT 32
static Navmesh navmesh_count = 0;
static struct Navmesh navmeshes[MAX_NAVMESH_COUNT];


Navmesh CreateTestNavmesh(void) {
    Navmesh id = navmesh_count++;
    struct Navmesh* navmesh = &navmeshes[id];

    navmesh->triangle_count = 4;
    navmesh->triangles[0] = (struct Triangle) {
	.p0={ .x=-1, .y=-1 }, .p1={ .x=1, .y=-1 }, .p2={ .x=1, .y=0 },
	.n0=3, .n1=-1, .n2=1,
	.t0=1, .t1=-1, .t2=-1
    };
    navmesh->triangles[1] = (struct Triangle) {
	.p0={ .x=-1, .y=-1 }, .p1={ .x=1, .y=0 }, .p2={ .x=-1, .y=0 },
	.n0=0, .n1=2, .n2=-1,
	.t0=-1, .t1=-1, .t2=-1
    };
    navmesh->triangles[2] = (struct Triangle) {
	.p0={ .x=-1, .y=0 }, .p1={ .x=1, .y=0 }, .p2={ .x=1, .y=1 },
	.n0=1, .n1=-1, .n2=3,
	.t0=-1, .t1=-1, .t2=-1
    };
    navmesh->triangles[3] = (struct Triangle) {
	.p0={ .x=-1, .y=0 }, .p1={ .x=1, .y=1 }, .p2={ .x=-1, .y=1 },
	.n0=2, .n1=0, .n2=-1,
	.t0=-1, .t1=0, .t2=-1
    };
    
    navmesh->transition_count = 2;
    navmesh->transitions[0] = (struct Transition) { 1, { .x=0, .y=1 }, { .x=1.5, .y=0.5 } };
    navmesh->transitions[1] = (struct Transition) { 0, { .x=0, .y=-1 }, { .x=1.5, .y=0.5 } };

    return id;
}


struct Agent {
    int navmesh;
    int triangle;
    union Vector2 position;
    union Vector2 velocity;
};


#define MAX_AGENT_COUNT 1024
static Agent agent_count = 0;
static struct Agent agents[MAX_AGENT_COUNT];


Agent CreateAgent(Navmesh navmesh_id) {
    Agent id = agent_count++;
    struct Agent* agent = &agents[id];
    agent->navmesh = navmesh_id;
    struct Navmesh navmesh = navmeshes[navmesh_id];
    agent->triangle = rand() % navmesh.triangle_count;
    struct Triangle t = navmesh.triangles[agent->triangle];
    agent->position = Scale2(Add2(t.p0, Add2(t.p1, t.p2)), 1.0 / 3.0);
    agent->velocity = Vector2(0, 0);
    return id;
}


void MoveAgent(Agent id, union Vector2 goal, float delta_time) {
    struct Agent* agent = &agents[id];

    agent->velocity = Add2(agent->velocity, goal);
    if (Magnitude2(agent->velocity) >= 1) {
	agent->velocity = Normalize2(agent->velocity);
    }
    agent->position = Add2(Scale2(agent->velocity, delta_time), agent->position);
    agent->velocity = Scale2(agent->velocity, 0.7);

    struct Navmesh navmesh = navmeshes[agent->navmesh];
    struct Triangle triangle = navmesh.triangles[agent->triangle];

    int next = -1;
    int tran = -1;
    union Vector2 a, b;

    struct {
	GLubyte r, g, b;
    } colors[3] = {
	{ 255, 0, 0 },
	{ 0, 255, 0 },
	{ 0, 0, 255 },
    };

    for (int i=0; i<3 /* aww */; i++) {
	a = triangle.p[i];
	b = triangle.p[(i + 1) % 3];
	float s = sign(agent->position, a, b);
	if (0 <= s) {
	    continue;
	}
	
	next = triangle.n[i];
	tran = triangle.t[i];
	if (next != -1) {
	    break;
	}
	
	union Vector2 nearest;
	float dist = distance_to_line(agent->position, a, b, &nearest);
	// TODO don't simply set this, but properly bounce/slide along walls and effect velocity
	agent->position = nearest;
	imColor3ub(255, 0, 0);
	imBegin(GL_LINES); {
	    imVertex2(agent->position);
	    imVertex2(nearest);
	} imEnd();
    }

    if (next < 0) {
	/* agent->position = nearest; */
	
    } else {
	/* printf("TRIDEX %d\n", next); */
	if (tran < 0) {

	} else {
	    struct Transition t = navmesh.transitions[tran];
	    struct Transition target = navmesh.transitions[t.target];
	    union Vector2 offset = Sub2(t.position, target.position);
	    agent->position = Sub2(agent->position, offset);
	}
	agent->triangle = next;
    }
}


void imDrawNavmesh(Navmesh id) {
    struct Navmesh navmesh = navmeshes[id];
    imColor3ub(100, 50, 0);
    for (int i=0; i<navmesh.triangle_count; ++i) {
	struct Triangle t = navmesh.triangles[i];
	imBegin(GL_LINE_LOOP); {
	    imVertex2(t.p0);
	    imVertex2(t.p1);
	    imVertex2(t.p2);
	} imEnd();
    }

    imColor3ub(0, 100, 50);
    for (int i=0; i<navmesh.transition_count; ++i) {
	struct Transition t = navmesh.transitions[i];
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
};


void imDrawAgent(Agent id, float radius) {
    struct Agent agent = agents[id];
    struct Navmesh navmesh = navmeshes[agent.navmesh];
    struct Triangle triangle = navmesh.triangles[agent.triangle];

    imColor3ub(255, 255, 0);
    imBegin(GL_LINE_LOOP); {
	imVertex2(triangle.p0);
	imVertex2(triangle.p1);
	imVertex2(triangle.p2);
    } imEnd();

    if (inside_triangle(agent.position, triangle)) {
	imColor3ub(0, 255, 255);
    } else {
	imColor3ub(255, 127, 0);
    }
    
    imModel(Translation(Vector3(agent.position.x, agent.position.y, 0)));
    imBegin(GL_LINE_LOOP); {
	imVertex2f(0, radius);
	imVertex2f(radius, 0);
	imVertex2f(0, -radius);
	imVertex2f(-radius, 0);
    } imEnd();
}
