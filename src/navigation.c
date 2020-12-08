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


struct Link {
    enum {
	EDGE,
	NEIGHBOR,
	PORTAL,
    } to;
    int target;
};


struct Triangle {
    union Triangle3 triangle;
    struct Link links[3];
};


struct Portal {
    int target;
    int triangle;
    int width;
    union Matrix4 transform_in;
    union Matrix4 transform_out;
};


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
static struct Navmesh navmeshes[MAX_NAVMESH_COUNT] = { 0 };


Navmesh LoadNavmesh(const char* filepath) {
    struct Navmesh navmesh = { 0 };
    
    {
	char nav_filepath[256] = { 0 };
	strcpy(nav_filepath, filepath);
	strcat(nav_filepath, ".navmesh");

	char* source = fopenstr(nav_filepath);
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
			       "%d,%d,%d "
			       "%d,%d,%d "
			       "%f,%f,%f "
			       "%f,%f,%f "
			       "%f,%f,%f",
			       &t.links[0].to, &t.links[1].to, &t.links[2].to,
			       &t.links[0].target, &t.links[1].target, &t.links[2].target,
			       &t.triangle.a.x, &t.triangle.a.y, &t.triangle.a.z,
			       &t.triangle.b.x, &t.triangle.b.y, &t.triangle.b.z,
			       &t.triangle.c.x, &t.triangle.c.y, &t.triangle.c.z);
			       

		if (s == 15) {
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
	strcat(ptl_filepath, ".portal_list");

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
		    struct Portal* p = &navmesh.portals[navmesh.portal_count++];
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

    Navmesh id = navmesh_count++;
    navmeshes[id] = navmesh;
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
    agent->position = Scale2(Add2(t.triangle.a.xy,
				  Add2(t.triangle.b.xy, t.triangle.c.xy)),
			     1.0 / 3.0);
    agent->velocity = Vector2(0, 0);
    agent->mass = 1.0f;
    agent->acceleration = Vector2(0, 0);
    return id;
}


void MoveAgent(Agent id, union Vector2 goal, float delta_time) {
    struct Agent* agent = &agents[id];
    struct Navmesh navmesh = navmeshes[agent->navmesh];

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
	    struct Link link = triangle.links[hit.i];
	    switch (link.to) {
	    case EDGE: {
		union Vector2 normal = Normalize2(Vector2(-(hit.b.y - hit.a.y),
							  hit.b.x - hit.a.x));
		float distance = DistanceToLine2(position, Line2(hit.a, hit.b));
		force = Add2(force, Scale2(normal, (distance + 0.01) * COLLISION_FORCE));
		break;
	    }
	    case NEIGHBOR: {
		agent->triangle = link.target;
		break;
	    }
	    case PORTAL: {
		struct Portal portal = navmesh.portals[link.target];
		struct Portal next_portal = navmesh.portals[portal.target];

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
    struct Navmesh navmesh = navmeshes[agent.navmesh];
    struct Triangle triangle = navmesh.triangles[agent.triangle];

    return From2To3(agent.position,
		    triangle.triangle.a,
		    triangle.triangle.b,
		    triangle.triangle.c);
}


void imDrawNavmesh(Navmesh id) {
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

    imColor3ub(0, 100, 50);
    for (int i=0; i<navmesh.portal_count; ++i) {
	imModel(navmesh.portals[i].transform_in);
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

    imModel(Translation(From2To3(agent.position,
				 triangle.triangle.a,
				 triangle.triangle.b,
				 triangle.triangle.c)));
    imBegin(GL_LINE_LOOP); {
	imVertex2f(0, radius);
	imVertex2f(radius, 0);
	imVertex2f(0, -radius);
	imVertex2f(-radius, 0);
    } imEnd();
}
