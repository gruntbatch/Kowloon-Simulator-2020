#pragma once


#include "area.h"
#include "mathematics.h"
#include "numbers.h"


void LoadNavmesh(Area area, const char* filepath);
void DrawNavmesh(Area navmesh);


void LoadPortals(Area area, const char* filepath);
void DrawPortals(Area portals);


typedef u32 Agent;
Agent CreateAgent(void);
void PlaceAgent(Agent agent, Area area);
void MoveAgent(Agent agent, union Vector2 goal, float delta_time);
union Vector3 GetAgentPosition(Agent agent);
void DrawAgent(Agent agent, float radius);
