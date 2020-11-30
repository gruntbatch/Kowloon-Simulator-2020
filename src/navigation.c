#include "navigation.h"


#include "immediate.h"
#include "logger.h"
#include <math.h>
#include "stdlib_plus.h"


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


static void draw_line(union Vector2 a, union Vector2 b) {
    imBegin(GL_LINES); {
	imVertex2(a);
	imVertex2(b);
    } imEnd();
}

#if 0
// The line to collide to, made from two points
line = lineEnd - lineStart;
// The line normal
normal = normalize(line.y, -line.x);
// The closest distance to the line from the origin (0, 0), is in the direction of the normal
d = dot(normal, lineStart);
// Check the distance from the line to the player start position
startDist = dot(normal, playerStart) - d;
// If the distance is negative, that means the player is 'behind' the line
// To correctly use the normal, if that is the case, invert the normal
if(startDist < 0.0f) { normal = -normal; d = -d;}
// Check the distance from the line to the player end position
// (using corrected normal if necessary, so playerStart is always in front of the line now)
endDist = dot(normal, playerEnd) - d;
// Check if playerEnd is behind the line
if (endDist < 0.0f) {
    // Here a collision has occured
    // Calculate the new position by moving playerEnd out to the line in the direction of the normal,
    // and a little bit further to counteract floating point inaccuracies
    actualEnd = playerEnd + normal * (-endDist + eps);
    // eps should be something less than a visible pixel, so it's not noticeable
}
#endif


static union Vector2 line_line(union Vector2 a, union Vector2 b, union Vector2 u, union Vector2 v) {
    // TODO This is magic to me
    #if 0
    // Line AB represented as a1x + b1y = c1 
    double a1 = B.second - A.second; 
    double b1 = A.first - B.first; 
    double c1 = a1*(A.first) + b1*(A.second); 
  
    // Line CD represented as a2x + b2y = c2 
    double a2 = D.second - C.second; 
    double b2 = C.first - D.first; 
    double c2 = a2*(C.first)+ b2*(C.second); 
  
    double determinant = a1*b2 - a2*b1; 
  
    if (determinant == 0) 
    { 
        // The lines are parallel. This is simplified 
        // by returning a pair of FLT_MAX 
        return make_pair(FLT_MAX, FLT_MAX); 
    } 
    else
    { 
        double x = (b2*c1 - b1*c2)/determinant; 
        double y = (a1*c2 - a2*c1)/determinant; 
        return make_pair(x, y); 
    }
    #endif
    float a1 = b.y - a.y;
    float b1 = a.x - b.x;

    float a2 = v.y - u.y;
    float b2 = u.x - v.x;

    float d = a1 * b2 - a2 * b1;

    if (d == 0) {
	return Vector2(INFINITY, INFINITY);
    } else {
	float c1 = a1 * (a.x) + b1 * (a.y);
	float c2 = a2 * (u.x) + b2 * (u.y);
	float x = (b2 * c1 - b1 * c2) / d;
	float y = (a1 * c2 - a2 * c1) / d;
	return Vector2(x, y);
    }
}


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
    float mass;
    union Vector2 acceleration;
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
    agent->mass = 1.0f;
    agent->acceleration = Vector2(0, 0);
    return id;
}


void MoveAgent(Agent id, union Vector2 goal, float delta_time) {
    struct Agent* agent = &agents[id];
    struct Navmesh navmesh = navmeshes[agent->navmesh];
    struct Triangle triangle = navmesh.triangles[agent->triangle];

    struct {
	GLubyte r, g, b;
    } colors[3] = {
	{ 255, 0, 0 },
	{ 0, 255, 0 },
	{ 0, 0, 255 },
    };

    union Vector2 force = Vector2(0, 0);

    /* Movement towards the goal */
    force = Add2(force, Scale2(Normalize2(goal), 13));

    /* Friction */
    force = Add2(force, Scale2(Negate2(Normalize2(agent->velocity)), Magnitude2(agent->velocity) * 7));

    /* Collision */
    int j;
    for (j=0; j<16; j++) {
	union Vector2 acceleration = Scale2(force, 1.0 / agent->mass);
	union Vector2 velocity = Add2(agent->velocity, Scale2(acceleration, delta_time));
	union Vector2 position = Add2(agent->position, Scale2(velocity, delta_time));

	struct {
	    float distance;
	    union Vector2 a, b;
	    int edge;
	} hit = { .distance=INFINITY };

	for (int i=0; i<3; i++) {
	    union Vector2 a, b;
	    a = triangle.p[i];
	    b = triangle.p[(i + 1) % 3];

	    float s = sign(position, a, b);
	    if (s <= 0) {
		union Vector2 point = line_line(agent->position, position, a, b);
		float distance = DistanceSquared2(agent->position, point);
		if (distance <= hit.distance) {
		    hit.distance = distance;
		    hit.a = a;
		    hit.b = b;
		    hit.edge = i;
		}
	    }
	}

	if (hit.edge != -1) {
	    imColor3ub(255, 0, 0);
	    draw_line(hit.a, hit.b);
	    
	    union Vector2 normal = Normalize2(Vector2(-(hit.b.y - hit.a.y), hit.b.x - hit.a.x));
	    float distance = distance_to_line(position, hit.a, hit.b, NULL);
	    force = Add2(force, Scale2(normal, (distance + 0.001) * 66));
	} else {
	    break;
	}
    }

    /* I'm not sure this works right for objects with masses other than 1.0 */
    agent->acceleration = Scale2(force, 1.0 / agent->mass);
    agent->velocity = Add2(agent->velocity, Scale2(agent->acceleration, delta_time));
    agent->position = Add2(agent->position, Scale2(agent->velocity, delta_time));
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

    /* imColor3ub(255, 255, 0); */
    /* imBegin(GL_LINE_LOOP); { */
    /* 	imVertex2(triangle.p0); */
    /* 	imVertex2(triangle.p1); */
    /* 	imVertex2(triangle.p2); */
    /* } imEnd(); */

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
