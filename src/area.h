#pragma once


#include "numbers.h"


#define MAX_AREA_COUNT 128
typedef u32 Area;
extern Area area_count;
Area LoadArea(const char* filepath);


void LoadScenery(Area area, const char* filepath);
void DrawScenery(Area area);
