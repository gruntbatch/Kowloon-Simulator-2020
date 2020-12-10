#include "retained.h"


#include "logger.h"
#include "SDL_plus.h"


struct String_GLuint64_Pair {
    char key[32];
    GLuint64 value;
};


#define MAX_KV_COUNT 512
static int kv_count = 0;
static struct String_GLuint64_Pair kvs[MAX_KV_COUNT];


GLuint64 rtLoadMeshAsset(const char* name) {
    for (int i=0; i<kv_count; i += 1) {
	struct String_GLuint64_Pair kv = kvs[i];
	if (strcmp(kv.key, name) == 0) {
	    return kv.value;
	}
    }

    if (kv_count < MAX_KV_COUNT) {
	struct String_GLuint64_Pair* kv = &kvs[kv_count++];
	strcpy(kv->key, name);

	char filepath[128] = "assets/meshes/";
	strcat(filepath, name);
	strcat(filepath, ".mesh");
	kv->value = rtLoadMesh(FromBase(filepath));
	return kv->value;
    } else {
	Warn("Unable to load any more meshes\n");
	return 0;
    }
}
