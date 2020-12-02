#pragma once


#include "mathematics.h"
#include "numbers.h"


typedef u32 Navmesh;
Navmesh LoadNavmesh(const char* filepath);
Navmesh CreateTestNavmesh(void);


typedef u32 Agent;
Agent CreateAgent(Navmesh navmesh);
void MoveAgent(Agent agent, union Vector2 goal, float delta_time);


void imDrawNavmesh(Navmesh navmesh);
void imDrawAgent(Agent agent, float radius);
