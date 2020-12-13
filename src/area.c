#include "area.h"


#include "immediate.h"
#include "logger.h"
#include "mathematics.h"
#include "retained.h"
#include "SDL_plus.h"
#include "stdlib_plus.h"
#include "string.h"


GLuint64 SCENERY_VERTEX_ARRAY;
static GLuint64 portal_mesh;
static GLuint lit_program;
static GLuint stencil_program;


enum Continue CreateAreaMeshes(void) {
    SCENERY_VERTEX_ARRAY = rtGenVertexArray();

    rtBindVertexArray(SCENERY_VERTEX_ARRAY);
    rtBegin(); {
	/* TODO Improve this */
	/* Why didn't my solid object work? */
	/* Y value should be greater, I think */
	imVertex3f(-1, 0.1, 0);
	imVertex3f( 1, 0.1, 0);
	imVertex3f(-1, 0.1, 3.2);
	imVertex3f( 1, 0.1, 3.2);
    } portal_mesh = rtEnd();

    lit_program = LoadProgram(FromBase("assets/shaders/vertex_lighting.vert"),
			      FromBase("assets/shaders/textured_vertex_color.frag"));
    stencil_program = LoadProgram(FromBase("assets/shaders/world_space.vert"),
				  FromBase("assets/shaders/vertex_color.frag"));

    return UP;
}


static u16 area_count = 0;
const Area INVALID_AREA = { .base=MAX_BASE_AREA_COUNT, .instance=MAX_INSTANCED_AREA_COUNT };
static int is_invalid(Area area) {
    return area.base == INVALID_AREA.base && area.instance == INVALID_AREA.instance;
}


Area LoadArea(const char* filepath) {
    if (area_count == MAX_BASE_AREA_COUNT) {
	Warn("Trying to load too many areas!\n");
	return (Area) { .id=0 };
    }

    Area id = { .base=area_count++, .instance=MAX_INSTANCED_AREA_COUNT };

    {
	char light_grid_filepath[256] = { 0 };
	strcpy(light_grid_filepath, filepath);
	strcat(light_grid_filepath, ".light_grid");
	LoadLightGrid(id, FromBase(light_grid_filepath));
    }
    
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


static u16 instance_count = 0;
static Area instances[MAX_INSTANCED_AREA_COUNT];


static Area instance_area(u16 base) {
    Area id = { .base=base, .instance=instance_count++ };
    instances[id.instance] = id;
    InstanceNetwork(id);
    return id;
}


Area InstanceArea(const Area area) {
    return instance_area(area.base);
}


void InstanceAreas(int count) {
    count = (MAX_INSTANCED_AREA_COUNT < count) ? MAX_INSTANCED_AREA_COUNT : count;

    int i = 0;
    /* Ensure each area is instanced at least once */
    for (; i<count && i<area_count; i++) {
	instance_area(i);
    }

    /* Randomly instance areas until count is reached */
    for (; i<count; i++) {
	instance_area(rand() % area_count);
    }
}


Area GetArea(u16 index) {
    return (union Area) { .base=index, .instance=0 };
}


Area GetAreaInstance(u16 index) {
    return instances[index];
}


struct Light {
    union Vector3 color;
    float energy;
    union Vector3 position;
    float distance;
};


#define MAX_LIGHT_COUNT 8


struct LightGrid {
    int light_count;
    int _x, _y, _z;
    struct Light lights[MAX_LIGHT_COUNT];
};


static struct LightGrid light_grids[MAX_BASE_AREA_COUNT];


void LoadLightGrid(Area id, const char* filepath) {
    char* source = fopenstr(filepath);
    if (!source) {
	Warn("Unable to open `%s`. Does it exist?\n", filepath);
	return;
    }

    struct LightGrid* light_grid = &light_grids[id.base];

    char* line = source;
    while (line) {
	char * endline = strchr(line, '\n');
	if (endline) {
	    *endline = '\0';

	    struct Light light;

	    int s = sscanf(line,
			   "%f "
			   "%f,%f,%f "
			   "%f,%f,%f",
			   &light.energy,
			   &light.color.r, &light.color.g, &light.color.b,
			   &light.position.x, &light.position.y, &light.position.z);
	    
	    if (s == 7) {
		light_grid->lights[light_grid->light_count++] = light;
	    }
	    
	    line = endline + 1;
	} else {
	    line = NULL;
	}
    }
    
    free(source);
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
static struct Network instanced_networks[MAX_INSTANCED_AREA_COUNT];


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


void InstanceNetwork(Area id) {
    instanced_networks[id.instance] = base_networks[id.base];
}


void LinkInstancedNetworks(void) {
    /* boi */
    /* Iterate through all instanced areas, marking all portals as
       unconnected */
    for (int instance_index=0; instance_index<instance_count; instance_index++) {
	struct Network* network = &instanced_networks[instance_index];
	for (int portal_index=0; portal_index<network->portal_count; portal_index++) {
	    network->portals[portal_index].destination = INVALID_AREA;
	}
    }
    
    /* Iterate through all instanced areas, connecting one random
       portal of each to another */
    for (int instance_index=0; instance_index<instance_count; instance_index++) {
	int instance_index_a = instance_index;
	int instance_index_b = (instance_index + 1) % instance_count;
	
	struct Network* network_a = &instanced_networks[instance_index_a];
	struct Network* network_b = &instanced_networks[instance_index_b];

	/* In theory, only one portal should be linked, so if we
	   happened to get the portal, just increment the portal
	   index */
	/* NOTE If an area only has a single portal, this won't work
	   properly */
	int portal_index_a = rand() % network_a->portal_count;
	if (!is_invalid(network_a->portals[portal_index_a].destination)) {
	    portal_index_a = (portal_index_a + 1) % network_a->portal_count;
	}
	int portal_index_b = rand() % network_b->portal_count;
	if (!is_invalid(network_b->portals[portal_index_b].destination)) {
	    portal_index_b = (portal_index_b + 1) % network_b->portal_count;
	}

	network_a->portals[portal_index_a].destination = instances[instance_index_b];
	network_a->portals[portal_index_a].portal_index = portal_index_b;
	
	network_b->portals[portal_index_b].destination = instances[instance_index_a];
	network_b->portals[portal_index_b].portal_index = portal_index_a;
    }

    /* Create a list of all unlinked portals, randomize that list,
       then link the portals together */
    int unlinked_portal_count = 0;
    struct Unlinked {
	int instance_index;
	int portal_index;
    } unlinked_portals[MAX_INSTANCED_AREA_COUNT * MAX_PORTAL_COUNT];
    for (int instance_index=0; instance_index<instance_count; instance_index++) {
	struct Network* network = &instanced_networks[instance_index];
	for (int portal_index=0; portal_index<network->portal_count; portal_index++) {
	    if (is_invalid(network->portals[portal_index].destination)) {
		unlinked_portals[unlinked_portal_count].instance_index = instance_index;
		unlinked_portals[unlinked_portal_count].portal_index = portal_index;
		unlinked_portal_count++;
	    }
	}
    }
    
    /* Adapted from https://stackoverflow.com/a/6127606 */
    if (unlinked_portal_count > 1) {
	size_t i;
	for (i=0; i<unlinked_portal_count-1; i++) {
	    size_t j = i + rand() / (RAND_MAX / (unlinked_portal_count - i) + 1);
	    struct Unlinked t = unlinked_portals[j];
	    unlinked_portals[j] = unlinked_portals[i];
	    unlinked_portals[i] = t;
	}
    }

    int even_unlinked_portal_count = unlinked_portal_count - (unlinked_portal_count % 2);
    for (int index=0; index<even_unlinked_portal_count; index += 2) {
	struct Unlinked a = unlinked_portals[index];
	struct Unlinked b = unlinked_portals[index + 1];

	struct Network* network_a = &instanced_networks[a.instance_index];
	struct Network* network_b = &instanced_networks[b.instance_index];

	network_a->portals[a.portal_index].destination = instances[b.instance_index];
	network_a->portals[a.portal_index].portal_index = b.portal_index;
	
	network_b->portals[b.portal_index].destination = instances[a.instance_index];
	network_b->portals[b.portal_index].portal_index = a.portal_index;
    }

    /* If there is a single portal left over, just link it to itself */
    /* TODO Modify this so that we end up with three portals linking to each other */
    if (unlinked_portal_count % 2) {
	struct Unlinked a = unlinked_portals[unlinked_portal_count - 1];

	struct Network* network_a = &instanced_networks[a.instance_index];

	network_a->portals[a.portal_index].destination = instances[a.instance_index];
	network_a->portals[a.portal_index].portal_index = a.portal_index;
    }
}


static struct Network* get_network(Area id) {
    if (id.instance == MAX_INSTANCED_AREA_COUNT) {
	return &base_networks[id.base];
    } else {
	return &instanced_networks[id.instance];
    }
}


void DrawNetwork(Area id) {
    struct Network* network = get_network(id);
    imColor3ub(0, 100, 50);
    for (int i=0; i<network->portal_count; ++i) {
	struct Portal* out_portal = &network->portals[i];
	imModel(out_portal->transform_in);
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
    imUseProgram(lit_program);
    struct LightGrid* light_grid = &light_grids[id.base];
    imSetLights(light_grid);
    struct Scenery* scenery = &sceneries[id.base];
    for (int i=0; i<scenery->static_count; ++i) {
	imModel(scenery->statics[i].transform);
	rtDrawArrays(GL_TRIANGLES, scenery->statics[i].mesh);
    }
}


#if 0
void DrawSceneryRecursively(Area id, int portal_index, union Matrix4 view, int depth) {
    struct Network* network = get_network(id);
    if (depth) {
	for (int i=0; i<network->portal_count; i++) {
	    if (i == portal_index) {
		continue;
	    }

	    struct Portal* out_portal = &network->portals[i];
	    struct Network* destination = get_network(out_portal->destination);
	    struct Portal* in_portal = &destination->portals[out_portal->portal_index];

	    union Matrix4 destination_view = MulM4(out_portal->transform_out, InvertM4(in_portal->transform_in));
	    destination_view = MulM4(view, destination_view);

	    /* imView(view); */
	    /* imModel(out_portal->transform_out); */
	    /* imUseProgram(stencil_program); */
	    /* rtDrawArrays(GL_TRIANGLE_STRIP, portal_mesh); */

	    DrawSceneryRecursively(out_portal->destination,
				   out_portal->portal_index,
				   destination_view,
				   depth - 1);
	}
    }
    imView(view);
    imClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    if (0 <= portal_index && portal_index <= network->portal_count) {
	struct Portal* in_portal = &network->portals[portal_index];
	imModel(in_portal->transform_in);
	imUseProgram(stencil_program);
	imDrawStencil();
	rtDrawArrays(GL_TRIANGLE_STRIP, portal_mesh);
    }
    imDrawColor();
    DrawScenery(id);
}
#endif


static void draw_children(Area id, int portal_index, union Matrix4 view, int depth) {
    if (depth) {
	struct Network* network = get_network(id);
	for (int i=0; i<network->portal_count; i++) {
	    if (i == portal_index) {
		continue;
	    }
	    
	    struct Portal* out_portal = &network->portals[i];
	    struct Network* destination = get_network(out_portal->destination);
	    struct Portal* in_portal = &destination->portals[out_portal->portal_index];

	    union Matrix4 destination_view = MulM4(out_portal->transform_out,
						   InvertM4(in_portal->transform_in));
	    destination_view = MulM4(view, destination_view);

	    draw_children(out_portal->destination,
			  out_portal->portal_index,
			  destination_view,
			  depth - 1);

	    imClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	    glDepthMask(GL_FALSE);
	    glStencilFunc(GL_ALWAYS, 1, 0xFF);
	    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	    imView(destination_view);
	    imModel(in_portal->transform_in);
	    imUseProgram(stencil_program);
	    rtDrawArrays(GL_TRIANGLE_STRIP, portal_mesh);
	    rtFlush();

	    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	    glDepthMask(GL_TRUE);
	    glStencilFunc(GL_EQUAL, 1, 0xFF);
	    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	    imUseProgram(lit_program);
	    DrawScenery(out_portal->destination);
	    rtFlush();
	}
    }
}


void DrawSceneryRecursively(Area id, int portal_index, union Matrix4 view, int depth) {
    rtBindVertexArray(SCENERY_VERTEX_ARRAY);

    glEnable(GL_STENCIL_TEST);
    draw_children(id, portal_index, view, depth);
    glDisable(GL_STENCIL_TEST);

    /* glStencilFunc(GL_NOTEQUAL, 1, 0xFF); */
    imView(view);
    imClear(GL_DEPTH_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    struct Network* network = get_network(id);
    for (int i=0; i<network->portal_count; i++) {
	imModel(network->portals[i].transform_out);
	rtDrawArrays(GL_TRIANGLE_STRIP, portal_mesh);
    }
    rtFlush();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    DrawScenery(id);
    rtFlush();
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

    /* TODO This should only be a Quaternion, but I don't want to fuck
       around with converting between Matrix4 and Quaternion, so we'll
       just use a Matrix4 instead */
    union Matrix4 rotation;
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

    agent->rotation = Rotation(AxisAngle(Vector3(0, 0, 1), to_radians((float)rand()/1000.0)));
    
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
		struct Network* network = get_network(agent->area_id);
		struct Portal* out_portal = &network->portals[cell->connection_index[hit.edge_index]];
		/* TODO Change the agent's area id */
		network = get_network(out_portal->destination);
		struct Portal* in_portal = &network->portals[out_portal->portal_index];

		union Matrix4 transform = MulM4(in_portal->transform_in, InvertM4(out_portal->transform_out));
		agent->position = Transform4(transform, Vector4(position.x, position.y, 0, 1)).xy;

		/* We do _not_ want to translate acceleration and
		   velocity, only scale and rotate them */
		transform.vectors[3] = Vector4(0, 0, 0, 1);
		agent->acceleration = Transform4(transform, Vector4(acceleration.x, acceleration.y, 0, 1)).xy;
		agent->velocity = Transform4(transform, Vector4(velocity.x, velocity.y, 0, 1)).xy;

		/* We invert the transform here. I'm not sure why, but
		   it seems to work */
		agent->rotation = MulM4(agent->rotation, InvertM4(transform));

		agent->area_id = out_portal->destination;
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


union Matrix4 GetAgentRotation(Agent id) {
    return agents[id].rotation;
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
