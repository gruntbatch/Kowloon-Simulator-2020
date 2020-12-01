#include "navigation.h"


#include "immediate.h"
#include "logger.h"
#include <math.h>
#include "stdlib_plus.h"


struct Link {
    enum {
	END,
	PORTAL,
	TRIANGLE,
    } to;
    int target;
};


struct Triangle {
    union Triangle2 triangle;
    struct Link links[3];
};


struct Portal {
    int target;
    int triangle;
    union Vector2 position;
    union Vector2 size;
};


static void draw_line(union Vector2 a, union Vector2 b) {
    imBegin(GL_LINES); {
	imVertex2(a);
	imVertex2(b);
    } imEnd();
}


#define MAX_TRIANGLE_COUNT 64
#define MAX_PORTAL_COUNT 8


struct Navmesh {
    int triangle_count;
    struct Triangle triangles[MAX_TRIANGLE_COUNT];
    int portal_count;
    struct Portal portals[MAX_PORTAL_COUNT];
};


#define MAX_NAVMESH_COUNT 32
static Navmesh navmesh_count = 0;
static struct Navmesh navmeshes[MAX_NAVMESH_COUNT];


Navmesh CreateTestNavmesh(void) {
    Navmesh id = navmesh_count++;
    struct Navmesh* navmesh = &navmeshes[id];

    navmesh->triangle_count = 4;
    navmesh->triangles[0] = (struct Triangle) {
	.triangle={ .a={ .x=-1, .y=-1 }, .b={ .x=1, .y=-1 }, .c={ .x=1, .y=0 } },
	.links={ { .to=PORTAL, .target=1 },
		 { 0 },
		 { .to=TRIANGLE, .target=1 } },
    };
    navmesh->triangles[1] = (struct Triangle) {
	.triangle={ .a={ .x=-1, .y=-1 }, .b={ .x=1, .y=0 }, .c={ .x=-1, .y=0 } },
	.links={ { .to=TRIANGLE, .target=0 },
		 { .to=TRIANGLE, .target=2 },
		 { 0 } },
    };
    navmesh->triangles[2] = (struct Triangle) {
	.triangle={ .a={ .x=-1, .y=0 }, .b={ .x=1, .y=0 }, .c={ .x=1, .y=1 } },
	.links={ { .to=TRIANGLE, .target=1 },
		 { 0 },
		 { .to=TRIANGLE, .target=3 } },
    };
    navmesh->triangles[3] = (struct Triangle) {
	.triangle={ .a={ .x=-1, .y=0 }, .b={ .x=1, .y=1 }, .c={ .x=-1, .y=1 } },
	.links={ { .to=TRIANGLE, .target=2 },
		 { .to=PORTAL, .target=0 },
		 { 0 } },
    };
    
    navmesh->portal_count = 2;
    navmesh->portals[0] = (struct Portal) {
	.target=1, .triangle=3,
	.position={ .x=0, .y=1 },
	.size={ .x=1.5, .y=0.5 },
    };
    navmesh->portals[1] = (struct Portal) {
	.target=0, .triangle=1,
	.position={ .x=0, .y=-1 },
	.size={ .x=1.5, .y=0.5 },
    };

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
    agent->position = Scale2(Add2(t.triangle.a, Add2(t.triangle.b, t.triangle.c)), 1.0 / 3.0);
    agent->velocity = Vector2(0, 0);
    agent->mass = 1.0f;
    agent->acceleration = Vector2(0, 0);
    return id;
}


void MoveAgent(Agent id, union Vector2 goal, float delta_time) {
    struct Agent* agent = &agents[id];
    struct Navmesh navmesh = navmeshes[agent->navmesh];
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
	struct Triangle triangle = navmesh.triangles[agent->triangle];

	struct {
	    float distance;
	    int i;
	    union Vector2 a, b;
	} hit = { .distance=INFINITY, .i=-1 };

	union Vector2 acceleration = Scale2(force, 1.0 / agent->mass);
	union Vector2 velocity = Add2(agent->velocity, Scale2(acceleration, delta_time));
	union Vector2 position = Add2(agent->position, Scale2(velocity, delta_time));

	for (int i=0; i<3 /* ilyt */; i++) {
	    union Vector2 a, b;
	    a = triangle.triangle.p[i];
	    b = triangle.triangle.p[(i + 1) % 3];

	    float s = Sign2(position, Line2(a, b));
	    if (s <= 0) {
		union Vector2 point = Intersect2(Line2(agent->position, position),
						 Line2(a, b));
		
		float distance = DistanceSquared2(agent->position, point);
		if (distance <= hit.distance) {
		    hit.distance = distance;
		    hit.a = a;
		    hit.b = b;
		    hit.i = i;
		}
	    }
	}

	if (hit.i != -1) {
	    struct Link link = triangle.links[hit.i];
	    switch (link.to) {
	    case END: {
		union Vector2 normal = Normalize2(Vector2(-(hit.b.y - hit.a.y),
							  hit.b.x - hit.a.x));
		float distance = DistanceToLine2(position, Line2(hit.a, hit.b));
		force = Add2(force, Scale2(normal, (distance + 0.01) * 66));
		break;
	    }
	    case PORTAL: {
		struct Portal portal = navmesh.portals[link.target];
		struct Portal next_portal = navmesh.portals[portal.target];
		union Vector2 offset = Sub2(portal.position, next_portal.position);
		agent->position = Sub2(agent->position, offset);

		agent->triangle = next_portal.triangle;
		break;
	    }
	    case TRIANGLE: {
		agent->triangle = link.target;
		break;
	    }
	    }
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
	    imVertex2(t.triangle.a);
	    imVertex2(t.triangle.b);
	    imVertex2(t.triangle.c);
	} imEnd();
    }

    imColor3ub(0, 100, 50);
    for (int i=0; i<navmesh.portal_count; ++i) {
	struct Portal t = navmesh.portals[i];
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
	imVertex2(triangle.triangle.a);
	imVertex2(triangle.triangle.b);
	imVertex2(triangle.triangle.c);
    } imEnd();

    if (InsideTriangle2(agent.position, triangle.triangle)) {
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
