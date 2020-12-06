#include "area.h"
#include "immediate.h"
#include "logger.h"
#include "retained.h"
#include <stdio.h>
#include <stdlib_plus.h>
#include <string.h>


struct Static {
    union Matrix4 transform;
    GLuint64 mesh;
};


#define MAX_STATIC_COUNT 128


struct Area {
    int scenery_count;
    struct Static scenery[MAX_STATIC_COUNT];
};


#define MAX_AREA_COUNT 128
static Area area_count = 0;
static struct Area areas[MAX_AREA_COUNT] = { 0 };


Area LoadArea(const char* filepath) {
    struct Area area = { 0 };

    {
	char xpt_filepath[256] = { 0 };
	strcpy(xpt_filepath, filepath);
	strcat(xpt_filepath, ".scenery");

	char* source = fopenstr(xpt_filepath);
	if (!source) {
	    Warn("Unable to open `%s`. Does it exist?\n", xpt_filepath);
	    return MAX_AREA_COUNT;
	}

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
		    struct Static* s = &area.scenery[area.scenery_count++];
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

    Area id = area_count++;
    areas[id] = area;
    return 0;
}


void DrawArea(Area id) {
    struct Area area = areas[id];
    for (int i=0; i<area.scenery_count; ++i) {
	struct Static s = area.scenery[i];
	imModel(s.transform);
	rtDrawArrays(GL_TRIANGLES, s.mesh);
    }
}
