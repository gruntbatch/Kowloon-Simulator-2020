#pragma once


#include "area.h"


void SpawnPlayer(Area area);
void PlayerWalkabout(float delta_time);
union Matrix4 GetPlayerView(void);
void DrawPlayer(float radius);
Area GetPlayerArea(void);
