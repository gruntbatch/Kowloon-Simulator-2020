#include "navigation.h"


#include "immediate.h"
#include "logger.h"
#include <math.h>
#include <stdio.h>
#include "stdlib_plus.h"
#include <string.h>


struct Link {
    enum {
	EDGE,
	NEIGHBOR,
	PORTAL,
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
    int width;
    union Vector2 position;
    union Quaternion rotation;
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


Navmesh LoadNavmesh(const char* filepath) {
    struct Navmesh navmesh;
    
    {
	char nav_filepath[256] = { 0 };
	strcpy(nav_filepath, filepath);
	strcat(nav_filepath, ".nav");

	char * source = fopenstr(nav_filepath);
	if (!source) {
	    Warn("Unable to open `%s`. Does it exist?\n", nav_filepath);
	    return MAX_NAVMESH_COUNT;
	}

	char* line = source;
	while (line) {
	    char * endline = strchr(line, '\n');
	    if (endline) {
		*endline = '\0';

		struct Triangle t;

		int s = sscanf(line,
			       "%*i "
			       "%d,%d,%d "
			       "%d,%d,%d "
			       "%f,%f,%*f "
			       "%f,%f,%*f "
			       "%f,%f,%*f",
			       &t.links[0].to, &t.links[1].to, &t.links[2].to,
			       &t.links[0].target, &t.links[1].target, &t.links[2].target,
			       &t.triangle.a.x, &t.triangle.a.y, /* &t.triangle.a.z, */
			       &t.triangle.b.x, &t.triangle.b.y, /* &t.triangle.b.z, */
			       &t.triangle.c.x, &t.triangle.c.y /*, &t.triangle.c.z */);
			       

		if (s == 12) {
		    navmesh.triangles[navmesh.triangle_count++] = t;
		}

		line = endline + 1;
	    } else {
		line = NULL;
	    }
	}
	
	free(source);
    }

    {
	char ptl_filepath[256] = { 0 };
	strcpy(ptl_filepath, filepath);
	strcat(ptl_filepath, ".ptl");

	char * source = fopenstr(ptl_filepath);
	if (!source) {
	    Warn("Unable to open `%s`. Does it exist?\n", ptl_filepath);
	    return MAX_NAVMESH_COUNT;
	}

	char* line = source;
	while (line) {
	    char * endline = strchr(line, '\n');
	    if (endline) {
		*endline = '\0';

		struct Portal p;

		int s = sscanf(line,
			       "%*i "
			       "%d %d "
			       "%f,%f,%*f "
			       "%f,%f,%f,%f",
			       &p.triangle, &p.width,
			       &p.position.x, &p.position.y, /* &p.position.z, */
			       &p.rotation.x, &p.rotation.y, &p.rotation.z, &p.rotation.w);
		
		if (s == 8) {
		    navmesh.portals[navmesh.portal_count++] = p;
		}
		
		line = endline + 1;
	    } else {
		line = NULL;
	    }
	}

	free(source);
    }

    Navmesh id = navmesh_count++;
    navmeshes[id] = navmesh;
    return id;
}


Navmesh CreateTestNavmesh(void) {
    Navmesh id = navmesh_count++;
    struct Navmesh* navmesh = &navmeshes[id];

    navmesh->triangle_count = 2;
    navmesh->triangles[0] = (struct Triangle) {
	.triangle={ .a={ .x=1, .y=1 }, .b={ .x=-1, .y=-1 }, .c={ .x=1, .y=-1 } },
	.links={ { .to=NEIGHBOR, .target=1 },
		 { .to=PORTAL, .target=1 },
		 { 0 } },
    };
    navmesh->triangles[1] = (struct Triangle) {
	.triangle={ .a={ .x=1, .y=1 }, .b={ .x=-1, .y=1 }, .c={ .x=-1, .y=-1 } },
	.links={ { .to=PORTAL, .target=0 },
		 { 0 },
		 { .to=NEIGHBOR, .target=0 } },
    };

    /* navmesh->portal_count = 2; */
    /* navmesh->portals[0] = (struct Portal) { */
	/* .target= */
    /* } */
    
    #if 0
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
	.target=1, .triangle=3, .width=1,
	.position={ .x=0, .y=1 },
	.rotation={ .x=0, .y=0, .z=0, .w=1 },
    };
    navmesh->portals[1] = (struct Portal) {
	.target=0, .triangle=1, .width=1,
	.position={ .x=0, .y=-1 },
	.rotation={ .x=0, .y=0, .z=0, .w=1 },
    };
    #endif

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
	    case EDGE: {
		union Vector2 normal = Normalize2(Vector2(-(hit.b.y - hit.a.y),
							  hit.b.x - hit.a.x));
		float distance = DistanceToLine2(position, Line2(hit.a, hit.b));
		force = Add2(force, Scale2(normal, (distance + 0.01) * 66));
		break;
	    }
	    case NEIGHBOR: {
		agent->triangle = link.target;
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
	struct Portal p = navmesh.portals[i];
	imModel(Transformation(Vector3(p.position.x, p.position.y, 0),
			       p.rotation,
			       Vector3(p.width, 1, 1)));
	imBegin(GL_LINE_LOOP); {
	    imVertex2f(-1, -1);
	    imVertex2f(1, -1);
	    imVertex2f(1, 1);
	    imVertex2f(-1, 1);
	} imEnd();
    }

    imModel(Matrix4(1));
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
