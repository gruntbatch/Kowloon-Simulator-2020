#pragma once


#include "mathematics.h"
#include "numbers.h"


#define MAX_BASE_AREA_COUNT 8
#define MAX_INSTANCED_AREA_COUNT 32


typedef union Area {
    struct {
	u16 base, instance;
    };
    u32 id;
} Area;


Area LoadArea(const char* filepath);
Area InstanceArea(const Area base);


void LoadNavmesh(Area id, const char* filepath);
void DrawNavmesh(Area id);


void LoadNetwork(Area id, const char* filepath);
void DrawNetwork(Area id);


void LoadScenery(Area id, const char* filepath);
void DrawScenery(Area id);


typedef u32 Agent;
Agent SpawnAgent(Area area);
void MoveAgent(Agent agent, union Vector2 goal, float delta_time);
union Vector3 GetAgentPosition(Agent agent);
void DrawAgent(Agent agent, float radius);
