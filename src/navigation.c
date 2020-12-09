#include "navigation.h"


#include "immediate.h"
#include "logger.h"
#include <math.h>
#include <stdio.h>
#include "stdlib_plus.h"
#include <string.h>


#define COLLISION_FORCE 64
#define FRICTION_FORCE 8
#define GOAL_FORCE 16


struct Triangle {
    union Triangle3 triangle;
    struct Neighbor {
	enum {
	    NOTHING,
	    TRIANGLE,
	    NETWORK,
	} to;
	int index;
    } neighbors[3];
};


#define MAX_TRIANGLE_COUNT 64


struct Navmesh {
    int triangle_count;
    struct Triangle triangles[MAX_TRIANGLE_COUNT];
};


static struct Navmesh navmeshes[MAX_AREA_COUNT] = { 0 };


void LoadNavmesh(Area id, const char* filepath) {
    char* source = fopenstr(filepath);
    if (!source) {
	Warn("Unable to open `%s`. Does it exist?\n", filepath);
	return;
    }

    struct Navmesh* navmesh = &navmeshes[id];

    char* line = source;
    while (line) {
	char * endline = strchr(line, '\n');
	if (endline) {
	    *endline = '\0';
	    
	    struct Triangle t;

	    int s = sscanf(line,
			   "%d,%d,%d "
			   "%d,%d,%d "
			   "%f,%f,%f "
			   "%f,%f,%f "
			   "%f,%f,%f",
			   &t.neighbors[0].to, &t.neighbors[1].to, &t.neighbors[2].to,
			   &t.neighbors[0].index, &t.neighbors[1].index, &t.neighbors[2].index,
			   &t.triangle.a.x, &t.triangle.a.y, &t.triangle.a.z,
			   &t.triangle.b.x, &t.triangle.b.y, &t.triangle.b.z,
			   &t.triangle.c.x, &t.triangle.c.y, &t.triangle.c.z);
	    
	    if (s == 15) {
		navmesh->triangles[navmesh->triangle_count++] = t;
	    }
	    
	    line = endline + 1;
	} else {
	    line = NULL;
	}
    }
    
    free(source);
}


void DrawNavmesh(Area id) {
    struct Navmesh navmesh = navmeshes[id];
    imModel(Matrix4(1));
    imColor3ub(100, 50, 0);
    for (int i=0; i<navmesh.triangle_count; ++i) {
	struct Triangle t = navmesh.triangles[i];
	imBegin(GL_LINE_LOOP); {
	    imVertex3(t.triangle.a);
	    imVertex3(t.triangle.b);
	    imVertex3(t.triangle.c);
	} imEnd();
    }
};


struct Portal {
    int target;
    int triangle;
    int width;
    union Matrix4 transform_in;
    union Matrix4 transform_out;
};


#define MAX_PORTAL_COUNT 8


struct Portals {
    int portal_count;
    struct Portal portals[MAX_PORTAL_COUNT];
};



/* struct Link { */
/*     Area  */
/* } */


/* struct Network { */
    
/* } */


/* This variable name is a crime against humanity */
static struct Portals portalss[MAX_AREA_COUNT];


void LoadPortals(Area id, const char* filepath) {
    char * source = fopenstr(filepath);
    if (!source) {
	Warn("Unable to open `%s`. Does it exist?\n", filepath);
	return;
    }
    
    struct Portals* portals = &portalss[id];

    char* line = source;
    while (line) {
	char * endline = strchr(line, '\n');
	if (endline) {
	    *endline = '\0';

	    int triangle = 0;
	    int width = 0;
	    union Vector3 position = { .x=0, .y=0, .z=0 };
	    union Quaternion rotation = { 0 };

	    int s = sscanf(line,
			   "%d %d "
			   "%f,%f,%f "
			   "%f,%f,%f,%f",
			   &triangle, &width,
			   &position.x, &position.y, &position.z,
			   &rotation.x, &rotation.y, &rotation.z, &rotation.w);
		
	    if (s == 9) {
		struct Portal* p = &portals->portals[portals->portal_count++];
		p->target = 0;
		p->triangle = triangle;
		p->width = width;
		p->transform_in = Transformation(position, rotation, Vector3(1, 1, 1));
		p->transform_out = Transformation(position,
						  MulQ(rotation, AxisAngle(Vector3(0, 0, 1), PI)),
						  Vector3(1, 1, 1));
	    }
	    
	    line = endline + 1;
	} else {
	    line = NULL;
	}
    }

    free(source);
}


void DrawPortals(Area id) {
    struct Portals portals = portalss[id];
    imColor3ub(0, 100, 50);
    for (int i=0; i<portals.portal_count; ++i) {
	imModel(portals.portals[i].transform_in);
	imBegin(GL_LINE_LOOP); {
	    imVertex2f(-1, -1);
	    imVertex2f(1, -1);
	    imVertex2f(1, 0);
	    imVertex2f(0, 1);
	    imVertex2f(-1, 0);
	} imEnd();
    }
}


struct Agent {
    Area area;
    int triangle;
    float mass;
    union Vector2 acceleration;
    union Vector2 velocity;
    union Vector2 position;
};


#define MAX_AGENT_COUNT 8
static Agent agent_count = 0;
static struct Agent agents[MAX_AGENT_COUNT];


Agent CreateAgent(void) {
    /* TODO Put guards against agent overpopulation */
    Agent id = agent_count++;
    struct Agent* agent = &agents[id];
    agent->area = MAX_AREA_COUNT;
    agent->triangle = MAX_TRIANGLE_COUNT;

    agent->mass = 1.0;
    agent->acceleration = Vector2(0, 0);
    agent->velocity = Vector2(0, 0);
    agent->position = Vector2(0, 0);
    return id;
}


void PlaceAgent(Agent agent_id, Area area_id) {
    struct Agent* agent = &agents[agent_id];
    agent->area = area_id;
    struct Navmesh navmesh = navmeshes[area_id];
    agent->triangle = rand() % navmesh.triangle_count;
    struct Triangle t = navmesh.triangles[agent->triangle];
    agent->position = Vector2((t.triangle.a.x + t.triangle.b.x + t.triangle.c.x) / 3.0,
			      (t.triangle.a.y + t.triangle.b.y + t.triangle.c.y) / 3.0);
}


void MoveAgent(Agent id, union Vector2 goal, float delta_time) {
    struct Agent* agent = &agents[id];
    struct Navmesh navmesh = navmeshes[agent->area];

    union Vector2 force = Vector2(0, 0);

    /* Movement towards the goal */
    force = Add2(force, Scale2(Normalize2(goal), GOAL_FORCE));

    /* Friction */
    force = Add2(force, Scale2(Negate2(Normalize2(agent->velocity)),
			       Magnitude2(agent->velocity) * FRICTION_FORCE));

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
	    a = triangle.triangle.p[i].xy;
	    b = triangle.triangle.p[(i + 1) % 3].xy;

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
	    struct Neighbor neighbor = triangle.neighbors[hit.i];
	    switch (neighbor.to) {
	    case NOTHING: {
		union Vector2 normal = Normalize2(Vector2(-(hit.b.y - hit.a.y),
							  hit.b.x - hit.a.x));
		float distance = DistanceToLine2(position, Line2(hit.a, hit.b));
		force = Add2(force, Scale2(normal, (distance + 0.01) * COLLISION_FORCE));
		break;
	    }
	    case TRIANGLE: {
		agent->triangle = neighbor.index;
		break;
	    }
	    case NETWORK: {
		struct Portals portals = portalss[agent->area];
		struct Portal portal = portals.portals[neighbor.index];
		struct Portal next_portal = portals.portals[portal.target];

		union Matrix4 transform = MulM4(next_portal.transform_in, InvertM4(portal.transform_out));
		agent->position = Transform4(transform, Vector4(position.x, position.y, 0, 1)).xy;

		/* We do _not_ want to translate acceleration and velocity, only scale and rotate them */
		transform.vectors[3] = Vector4(0, 0, 0, 1);
		agent->acceleration = Transform4(transform, Vector4(acceleration.x, acceleration.y, 0, 1)).xy;
		agent->velocity = Transform4(transform, Vector4(velocity.x, velocity.y, 0, 1)).xy;

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


union Vector3 GetAgentPosition(Agent id) {
    struct Agent agent = agents[id];
    struct Navmesh navmesh = navmeshes[agent.area];
    struct Triangle triangle = navmesh.triangles[agent.triangle];

    return From2To3(agent.position,
		    triangle.triangle.a,
		    triangle.triangle.b,
		    triangle.triangle.c);
}


void DrawAgent(Agent id, float radius) {
    struct Agent agent = agents[id];
    struct Navmesh navmesh = navmeshes[agent.area];
    struct Triangle triangle = navmesh.triangles[agent.triangle];

    imModel(Matrix4(1));
    imColor3ub(255, 255, 0);
    imBegin(GL_LINE_LOOP); {
	imVertex3(triangle.triangle.a);
	imVertex3(triangle.triangle.b);
	imVertex3(triangle.triangle.c);
    } imEnd();

    union Triangle3 t = triangle.triangle;
    if (InsideTriangle2(agent.position, t.a.xy, t.b.xy, t.c.xy)) {
	imColor3ub(0, 255, 255);
    } else {
	imColor3ub(255, 127, 0);
    }

    imModel(Translation(From2To3(agent.position, t.a, t.b, t.c)));
    imBegin(GL_LINE_LOOP); {
	imVertex2f(0, radius);
	imVertex2f(radius, 0);
	imVertex2f(0, -radius);
	imVertex2f(-radius, 0);
    } imEnd();
}
