#pragma once


#include "GL_plus.h"
#include "ladder.h"
#include "mathematics.h"
#include "numbers.h"


#define MAX_BASE_AREA_COUNT 8
#define MAX_INSTANCED_AREA_COUNT 32


extern GLuint64 SCENERY_VERTEX_ARRAY;


enum Continue CreateAreaMeshes(void);


typedef union Area {
    struct {
	u16 base, instance;
    };
    u32 id;
} Area;


Area LoadArea(const char* filepath);
Area InstanceArea(const Area base);
void InstanceAreas(int count);
Area GetArea(u16 index);
Area GetAreaInstance(u16 index);


void LoadLightGrid(Area id, const char* filepath);


void LoadNavmesh(Area id, const char* filepath);
void DrawNavmesh(Area id);


void LoadNetwork(Area id, const char* filepath);
void InstanceNetwork(Area id);
void LinkInstancedNetworks(void);
void DrawNetwork(Area id);


void LoadScenery(Area id, const char* filepath);
void DrawScenery(Area id);
void DrawSceneryRecursively(Area id, int portal_index, union Matrix4 view, int depth);


typedef u32 Agent;
Agent SpawnAgent(Area area);
void MoveAgent(Agent agent, union Vector2 goal, float delta_time);
union Vector3 GetAgentPosition(Agent agent);
union Matrix4 GetAgentRotation(Agent agent);
Area GetAgentArea(Agent agent);
void DrawAgent(Agent agent, float radius);
