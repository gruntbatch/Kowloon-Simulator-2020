#pragma once


#include "mathematics.h"
#include "numbers.h"


typedef u32 Navmesh;
Navmesh CreateTestNavmesh(void);


typedef u32 Agent;
Agent CreateAgent(Navmesh navmesh);
void MoveAgent(Agent agent, union Vector2 input, float delta_time);


void imDrawNavmesh(Navmesh navmesh);
void imDrawAgent(Agent agent, float radius);
