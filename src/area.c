#include "area.h"
#include "immediate.h"
#include "logger.h"
#include "navigation.h"
#include "retained.h"
#include "SDL_plus.h"
#include <stdio.h>
#include <stdlib_plus.h>
#include <string.h>


Area area_count = 0;


Area LoadArea(const char* filepath) {
    Area id = area_count++;

    {
	char navmesh_filepath[256] = { 0 };
	strcpy(navmesh_filepath, filepath);
	strcat(navmesh_filepath, ".navmesh");
	LoadNavmesh(id, FromBase(navmesh_filepath));
    }

    {
	char portals_filepath[256] = { 0 };
	strcpy(portals_filepath, filepath);
	strcat(portals_filepath, ".portals");
	LoadPortals(id, FromBase(portals_filepath));
    }

    {
	char scenery_filepath[256] = { 0 };
	strcpy(scenery_filepath, filepath);
	strcat(scenery_filepath, ".scenery");
	LoadScenery(id, FromBase(scenery_filepath));
    }

    return id;
}


struct Static {
    union Matrix4 transform;
    GLuint64 mesh;
};


#define MAX_STATIC_COUNT 128
struct Scenery {
    int static_count;
    struct Static statics[MAX_STATIC_COUNT];
};


static struct Scenery sceneries[MAX_AREA_COUNT];


void LoadScenery(Area id, const char* filepath) {
    char* source = fopenstr(filepath);
    if (!source) {
	Warn("Unable to open `%s`. Does it exist?\n", filepath);
	return;
    }

    struct Scenery* scenery = &sceneries[id];

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
		/* Log("Looking for mesh `%s`\n", mesh_name); */
		struct Static* s = &scenery->statics[scenery->static_count++];
		s->transform = Transformation(translation, rotation, scale);
		s->mesh = rtLoadMeshAsset(mesh_name);
	    }

	    line = endline + 1;
	} else {
	    line = NULL;
	}
    }
    
    free(source);
}


void DrawScenery(Area id) {
    struct Scenery scenery = sceneries[id];
    for (int i=0; i<scenery.static_count; ++i) {
	struct Static s = scenery.statics[i];
	imModel(s.transform);
	rtDrawArrays(GL_TRIANGLES, s.mesh);
    }
}
