#include "area.h"


#include "immediate.h"
#include "logger.h"
#include "mathematics.h"
#include "retained.h"
#include "SDL_plus.h"
#include "stdlib_plus.h"
#include "string.h"


static u16 area_count = 0;


Area LoadArea(const char* filepath) {
    Area id = { .base=area_count++, .instance=0 };
    
    {
	char navmesh_filepath[256] = { 0 };
	strcpy(navmesh_filepath, filepath);
	strcat(navmesh_filepath, ".navmesh");
	LoadNavmesh(id, FromBase(navmesh_filepath));
    }

    {
	char network_filepath[256] = { 0 };
	strcpy(network_filepath, filepath);
	strcat(network_filepath, ".network");
	LoadNetwork(id, FromBase(network_filepath));
    }

    {
	char scenery_filepath[256] = { 0 };
	strcpy(scenery_filepath, filepath);
	strcat(scenery_filepath, ".scenery");
	LoadScenery(id, FromBase(scenery_filepath));
    }
    
    return id;
}


Area GetArea(u16 index) {
    return (union Area) { .base=index, .instance=0 };
}


struct Cell {
    union Triangle3 triangle;
    enum {
	NOTHING,
	CELL,
	NETWORK,
    } connected_to[3];
    int connection_index[3];
};


#define MAX_CELL_COUNT 64
struct Navmesh {
    int cell_count;
    struct Cell cells[MAX_CELL_COUNT];
};


static struct Navmesh navmeshes[MAX_BASE_AREA_COUNT];


void LoadNavmesh(Area id, const char* filepath) {
    char* source = fopenstr(filepath);
    if (!source) {
	Warn("Unable to open `%s`. Does it exist?\n", filepath);
	return;
    }

    struct Navmesh* navmesh = &navmeshes[id.base];

    char* line = source;
    while (line) {
	char * endline = strchr(line, '\n');
	if (endline) {
	    *endline = '\0';

	    struct Cell cell;

	    int s = sscanf(line,
			   "%d,%d,%d "
			   "%d,%d,%d "
			   "%f,%f,%f "
			   "%f,%f,%f "
			   "%f,%f,%f",
			   &cell.connected_to[0], &cell.connected_to[1], &cell.connected_to[2],
			   &cell.connection_index[0], &cell.connection_index[1], &cell.connection_index[2],
			   &cell.triangle.a.x, &cell.triangle.a.y, &cell.triangle.a.z,
			   &cell.triangle.b.x, &cell.triangle.b.y, &cell.triangle.b.z,
			   &cell.triangle.c.x, &cell.triangle.c.y, &cell.triangle.c.z);
	    
	    if (s == 15) {
		navmesh->cells[navmesh->cell_count++] = cell;
	    }
	    
	    line = endline + 1;
	} else {
	    line = NULL;
	}
    }
    
    free(source);
}


void DrawNavmesh(Area id) {
    struct Navmesh* navmesh = &navmeshes[id.base];
    imModel(Matrix4(1));
    imColor3ub(100, 50, 0);
    for (int i=0; i<navmesh->cell_count; ++i) {
	union Triangle3 triangle = navmesh->cells[i].triangle;
	imBegin(GL_LINE_LOOP); {
	    imVertex3(triangle.a);
	    imVertex3(triangle.b);
	    imVertex3(triangle.c);
	} imEnd();
    }
};


struct Portal {
    int width;
    int portal_index;
    Area destination;
    union Matrix4 transform_out;
    union Matrix4 transform_in;
    int cell_index;
};


#define MAX_PORTAL_COUNT 8


struct Network {
    int portal_count;
    struct Portal portals[MAX_PORTAL_COUNT];
};


static struct Network base_networks[MAX_BASE_AREA_COUNT];
/* static struct Network instanced_networks[MAX_INSTANCED_AREA_COUNT]; */


void LoadNetwork(Area id, const char* filepath) {
    char * source = fopenstr(filepath);
    if (!source) {
	Warn("Unable to open `%s`. Does it exist?\n", filepath);
	return;
    }
    
    struct Network* network = &base_networks[id.base];

    char* line = source;
    while (line) {
	char * endline = strchr(line, '\n');
	if (endline) {
	    *endline = '\0';

	    int width = 0;
	    int cell_index = 0;
	    union Vector3 position = { .x=0, .y=0, .z=0 };
	    union Quaternion rotation = { 0 };

	    int s = sscanf(line,
			   "%d %d "
			   "%f,%f,%f "
			   "%f,%f,%f,%f",
			   &cell_index, &width,
			   &position.x, &position.y, &position.z,
			   &rotation.x, &rotation.y, &rotation.z, &rotation.w);
		
	    if (s == 9) {
		struct Portal* p = &network->portals[network->portal_count++];
		p->width = width;
		p->portal_index = 0;
		p->destination = (Area) { .id=0 };
		p->transform_out = Transformation(position,
						  MulQ(rotation, AxisAngle(Vector3(0, 0, 1), PI)),
						  Vector3(1, 1, 1));
		p->transform_in = Transformation(position, rotation, Vector3(1, 1, 1));
		p->cell_index = cell_index;
	    }
	    
	    line = endline + 1;
	} else {
	    line = NULL;
	}
    }

    free(source);
}


void DrawNetwork(Area id) {
    /* TODO correctly select base or instanced network */
    struct Network* network = &base_networks[id.base];
    imColor3ub(0, 100, 50);
    for (int i=0; i<network->portal_count; ++i) {
	imModel(network->portals[i].transform_in);
	imBegin(GL_LINE_LOOP); {
	    imVertex2f(-1, -1);
	    imVertex2f(1, -1);
	    imVertex2f(1, 0);
	    imVertex2f(0, 1);
	    imVertex2f(-1, 0);
	} imEnd();
    }
}


#define MAX_STATIC_COUNT 128
struct Scenery {
    int static_count;
    struct {
	union Matrix4 transform;
	GLuint64 mesh;
    } statics[MAX_STATIC_COUNT];
};


static struct Scenery sceneries[MAX_BASE_AREA_COUNT];


void LoadScenery(Area id, const char* filepath) {
    char* source = fopenstr(filepath);
    if (!source) {
	Warn("Unable to open `%s`. Does it exist?\n", filepath);
	return;
    }

    struct Scenery* scenery = &sceneries[id.base];

    char* line = source;
    while (line) {
	char * endline = strchr(line, '\n');
	if (endline) {
	    *endline = '\0';
	    
	    char mesh_name[32];

	    union Vector3 translation;
	    union Quaternion rotation;
	    union Vector3 scale;
	    
	    int s = sscanf(line,
			   "%s "
			   "%f,%f,%f "
			   "%f,%f,%f,%f "
			   "%f,%f,%f",
			   mesh_name,
			   &translation.x, &translation.y, &translation.z,
			   &rotation.x, &rotation.y, &rotation.z, &rotation.w,
			   &scale.x, &scale.y, &scale.z);
	    
	    if (s == 11) {
		scenery->statics[scenery->static_count].transform = Transformation(translation, rotation, scale);
		scenery->statics[scenery->static_count].mesh = rtLoadMeshAsset(mesh_name);
		scenery->static_count++;
	    }

	    line = endline + 1;
	} else {
	    line = NULL;
	}
    }
    
    free(source);
}


void DrawScenery(Area id) {
    struct Scenery* scenery = &sceneries[id.base];
    for (int i=0; i<scenery->static_count; ++i) {
	imModel(scenery->statics[i].transform);
	rtDrawArrays(GL_TRIANGLES, scenery->statics[i].mesh);
    }
}


void DrawSceneryTransformed(Area id, union Matrix4 transform) {
    struct Scenery* scenery = &sceneries[id.base];
    for (int i=0; i<scenery->static_count; ++i) {
	imModel(MulM4(transform, scenery->statics[i].transform));
	rtDrawArrays(GL_TRIANGLES, scenery->statics[i].mesh);
    }
}


void DrawSceneryRecursively(Area id, int portal_index, union Matrix4 transform, int depth) {
    /* TODO Clear depth buffer */
    if (depth) {
	struct Network* network = &base_networks[id.base];
	for (int i=0; i<network->portal_count; i++) {
	/* for (int i=0; i<1; i++) { */
	    if (i == portal_index) {
		continue;
	    }

	    struct Portal* out_portal = &network->portals[i];
	    struct Portal* in_portal = &network->portals[out_portal->portal_index];
	    
	    union Matrix4 transform1 = MulM4(out_portal->transform_out, InvertM4(in_portal->transform_in));
	    transform1 = MulM4(transform, transform1);

	    /* TODO use out_portal->area_id */
	    DrawSceneryRecursively(id,
				   out_portal->portal_index,
				   transform1,
				   depth - 1);
	}
    }
    DrawSceneryTransformed(id, transform);
}


#define COLLISION_FORCE 64.0f
#define FRICTION_FORCE 8.0f
#define GOAL_FORCE 16.0f
#define MAX_COLLISION_ITERATION_COUNT 16


struct Agent {
    Area area_id;
    int cell_index;
    float mass;
    union Vector2 acceleration;
    union Vector2 velocity;
    union Vector2 position;
};


#define MAX_AGENT_COUNT 8
static Agent agent_count = 0;
static struct Agent agents[MAX_AGENT_COUNT];


Agent SpawnAgent(Area area_id) {
    Agent agent_id = agent_count++;
    /* TODO Perform bounds checks */
    struct Agent* agent = &agents[agent_id];

    agent->area_id = area_id;

    struct Navmesh* navmesh = &navmeshes[area_id.base];
    agent->cell_index = rand() % navmesh->cell_count;

    agent->mass = 1.0;
    agent->acceleration = Vector2(0, 0);
    agent->velocity = Vector2(0, 0);

    union Triangle3 triangle = navmesh->cells[agent->cell_index].triangle;
    agent->position = Vector2((triangle.a.x + triangle.b.x + triangle.c.x) / 3.0,
			      (triangle.a.y + triangle.b.y + triangle.c.y) / 3.0);
    
    return agent_id;
}


void MoveAgent(Agent agent_id, union Vector2 goal, float delta_time) {
    /* Start each frame with 0 force */
    union Vector2 force = Vector2(0, 0);

    /* Move towards the goal */
    force = Add2(force, Scale2(Normalize2(goal), GOAL_FORCE));

    /* Factor in friction based on the agent's velocity */
    struct Agent* agent = &agents[agent_id];
    force = Add2(force, Scale2(Negate2(Normalize2(agent->velocity)),
			       Magnitude2(agent->velocity) * FRICTION_FORCE));

    /* Collision forces */
    int iteration_count;
    for (iteration_count=0; iteration_count<MAX_COLLISION_ITERATION_COUNT; iteration_count++) {
	union Vector2 acceleration = Scale2(force, 1.0 / agent->mass);
	union Vector2 velocity = Add2(agent->velocity, Scale2(acceleration, delta_time));
	union Vector2 position = Add2(agent->position, Scale2(velocity, delta_time));

	struct Navmesh* navmesh = &navmeshes[agent->area_id.base];
	struct Cell* cell = &navmesh->cells[agent->cell_index];
	
	struct {
	    float distance;
	    int edge_index;
	    union Vector2 a, b;
	} hit = { .distance=INFINITY, .edge_index=-1 };

	for (int edge_index=0; edge_index<3; edge_index++) {
	    union Vector2 a, b;
	    a = cell->triangle.p[edge_index].xy;
	    b = cell->triangle.p[(edge_index + 1) % 3].xy;

	    float s = Sign2(position, Line2(a, b));
	    if (s <= 0) {
		union Vector2 point = Intersect2(Line2(agent->position, position),
						 Line2(a, b));

		float distance = DistanceSquared2(agent->position, point);
		if (distance <= hit.distance) {
		    hit.distance = distance;
		    hit.edge_index = edge_index;
		    hit.a = a;
		    hit.b = b;
		}
	    }
	}

	if (hit.edge_index != -1) {
	    switch(cell->connected_to[hit.edge_index]) {
	    case NOTHING: {
		union Vector2 normal = Normalize2(Vector2(-(hit.b.y - hit.a.y),
							  hit.b.x - hit.a.x));
		float distance = DistanceToLine2(position, Line2(hit.a, hit.b));
		force = Add2(force, Scale2(normal, (distance + 0.01) * COLLISION_FORCE));
		break;
	    }
	    case CELL: {
		agent->cell_index = cell->connection_index[hit.edge_index];
		break;
	    }
	    case NETWORK: {
		struct Network* network = &base_networks[agent->area_id.base];
		struct Portal* out_portal = &network->portals[cell->connection_index[hit.edge_index]];
		/* TODO Change the agent's area id */
		struct Portal* in_portal = &network->portals[out_portal->portal_index];

		union Matrix4 transform = MulM4(in_portal->transform_in, InvertM4(out_portal->transform_out));
		agent->position = Transform4(transform, Vector4(position.x, position.y, 0, 1)).xy;

		/* We do _not_ want to translate acceleration and velocity, only scale and rotate them */
		transform.vectors[3] = Vector4(0, 0, 0, 1);
		agent->acceleration = Transform4(transform, Vector4(acceleration.x, acceleration.y, 0, 1)).xy;
		agent->velocity = Transform4(transform, Vector4(velocity.x, velocity.y, 0, 1)).xy;

		agent->cell_index = in_portal->cell_index;
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


Area GetAgentArea(Agent id) {
    return agents[id].area_id;
}


union Vector3 GetAgentPosition(Agent id) {
    struct Agent* agent = &agents[id];
    union Triangle3 triangle = navmeshes[agent->area_id.base].cells[agent->cell_index].triangle;

    return From2To3(agent->position, triangle.a, triangle.b, triangle.c);
}


void DrawAgent(Agent id, float radius) {
    struct Agent* agent = &agents[id];
    union Triangle3 triangle = navmeshes[agent->area_id.base].cells[agent->cell_index].triangle;

    imModel(Matrix4(1));
    imColor3ub(255, 255, 0);
    imBegin(GL_LINE_LOOP); {
	imVertex3(triangle.a);
	imVertex3(triangle.b);
	imVertex3(triangle.c);
    } imEnd();

    if (InsideTriangle2(agent->position, triangle.a.xy, triangle.b.xy, triangle.c.xy)) {
	imColor3ub(0, 255, 255);
    } else {
	imColor3ub(255, 127, 0);
    }

    imModel(Translation(From2To3(agent->position, triangle.a, triangle.b, triangle.c)));
    imBegin(GL_LINE_LOOP); {
	imVertex2f(0, radius);
	imVertex2f(radius, 0);
	imVertex2f(0, -radius);
	imVertex2f(-radius, 0);
    } imEnd();
}
