#include "immediate.h"
#include "retained.h"


#include "logger.h"
#include "stdlib_plus.h"
#include <stdio.h>
#include <string.h>


struct UniformBuffer MATRICES = { .name="Matrices",
                                  .size=3 * sizeof(union Matrix4),
                                  .bind=0,
				  .id=0 };


void imInitTransformBuffer(void) {
    glLogErrors();

    /* TODO Since this can fail, should it return a Maybe? */
    glGenBuffers(1, &MATRICES.id);

    glBindBuffer(GL_UNIFORM_BUFFER, MATRICES.id); {
        glBufferData(GL_UNIFORM_BUFFER,
                     MATRICES.size,
                     NULL,
                     GL_DYNAMIC_DRAW);
    } glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER,
                     MATRICES.bind,
                     MATRICES.id);

    glLogErrors();
}


#define VERTEX_MAX_COUNT U16_MAX


struct Vertex {
    union Vector3 position;
    union Vector3 normal;
    union Vector4 color;
    union Vector2 texcoord;
};


static GLuint vertex_count;
static struct Vertex vertices[VERTEX_MAX_COUNT];
static struct Vertex current_vertex;


static GLuint64 internal_vertex_array;


void imInitInternalVertexArray(void) {
    internal_vertex_array = rtGenVertexArray();
}


void imBindVertexArray(void) {
    rtBindVertexArray(internal_vertex_array);
}


#define COMMAND_MAX_COUNT 4096


enum CommandType {
    COMMAND_ACTIVE_TEXTURE,
    COMMAND_ANY,
    COMMAND_INSTANCED_PRIMITIVE,
    COMMAND_MODEL,
    COMMAND_PRIMITIVE,
    COMMAND_PROGRAM,
    COMMAND_PROJECTION,
    COMMAND_BIND_TEXTURE,
    COMMAND_VIEW,
};


struct Command {
    enum CommandType type;
    union {
        struct {
            GLenum texture;
        } active_texture;
        union Matrix4 model;
        struct {
            GLuint vertex_array;
            GLenum mode;
            GLint first;
            GLsizei count;
            GLsizei instancecount;
        } primitive;
        struct {
            GLuint id;
        } program;
        union Matrix4 projection;
        struct {
            GLenum target;
            GLuint id;
            GLenum slot;
        } bind_texture;
        union Matrix4 view;
    };
};


static GLuint command_count;
static struct Command commands[COMMAND_MAX_COUNT];
static struct Command current_command;


static enum CommandType current_mode = COMMAND_ANY;


#define MODE_MUST_BE(mode)                      \
    if (current_mode != mode) {                 \
        return;                                 \
    }
#define MODE_MUST_BE_OR_ERR(mode, err)          \
    if (current_mode != mode) {                 \
        return err;                             \
    }
#define ADVANCE_COMMAND() \
    if (command_count < COMMAND_MAX_COUNT) {            \
        commands[command_count++] = current_command;     \
    }


void imModel(union Matrix4 model) {
    MODE_MUST_BE(COMMAND_ANY);

    current_command.type = COMMAND_MODEL;
    current_command.model = model;

    ADVANCE_COMMAND();
}


void imView(union Matrix4 view) {
    MODE_MUST_BE(COMMAND_ANY);

    current_command.type = COMMAND_VIEW;
    current_command.view = view;

    ADVANCE_COMMAND();
}


void imProjection(union Matrix4 projection) {
    MODE_MUST_BE(COMMAND_ANY);

    current_command.type = COMMAND_PROJECTION;
    current_command.projection = projection;

    ADVANCE_COMMAND();
}


void imActiveTexture(GLenum texture) {
    MODE_MUST_BE(COMMAND_ANY);

    current_command.type = COMMAND_ACTIVE_TEXTURE;
    current_command.active_texture.texture = texture;

    ADVANCE_COMMAND();
}


void imBindTexture(GLenum target, GLuint texture) {
    MODE_MUST_BE(COMMAND_ANY);

    current_command.type = COMMAND_BIND_TEXTURE;
    current_command.bind_texture.target = target;
    current_command.bind_texture.id = texture;

    ADVANCE_COMMAND();
}


void imUseProgram(GLuint program) {
    MODE_MUST_BE(COMMAND_ANY);

    current_command.type = COMMAND_PROGRAM;
    current_command.program.id = program ;

    ADVANCE_COMMAND();
}


void imBegin(GLenum mode) {
    MODE_MUST_BE(COMMAND_ANY);
    current_mode = COMMAND_PRIMITIVE;

    current_command.type = COMMAND_PRIMITIVE;
    current_command.primitive.mode = mode;
    current_command.primitive.first = vertex_count;
    current_command.primitive.count = 0;
}


void imEnd(void) {
    MODE_MUST_BE(COMMAND_PRIMITIVE);
    current_mode = COMMAND_ANY;

    ADVANCE_COMMAND();
}


void imColor(union Vector4 color) {
    current_vertex.color = color;
}


void imColor3f(GLfloat r, GLfloat g, GLfloat b) {
    current_vertex.color.r = r;
    current_vertex.color.g = g;
    current_vertex.color.b = b;
    current_vertex.color.a = 1.0f;
}


void imColor3ub(GLubyte r, GLubyte g, GLubyte b) {
    current_vertex.color.r = (GLfloat)r / 255.0;
    current_vertex.color.g = (GLfloat)g / 255.0;
    current_vertex.color.b = (GLfloat)b / 255.0;
    current_vertex.color.a = 1.0f;
}


void imColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    current_vertex.color.r = r;
    current_vertex.color.g = g;
    current_vertex.color.b = b;
    current_vertex.color.a = a;
}


void imNormal3(union Vector3 normal) {
    imNormal3f(normal.x, normal.y, normal.z);
}


void imNormal3f(GLfloat x, GLfloat y, GLfloat z) {
    current_vertex.normal.x = x;
    current_vertex.normal.y = y;
    current_vertex.normal.z = z;
}


void imTexCoord(union Vector2 texcoord) {
    current_vertex.texcoord = texcoord;
}


void imTexCoord2f(GLfloat u, GLfloat v) {
    current_vertex.texcoord.u = u;
    current_vertex.texcoord.v = v;
}


void imVertex2(union Vector2 position) {
    imVertex3f(position.x, position.y, 0.0f);
}


void imVertex2f(GLfloat x, GLfloat y) {
    imVertex3f(x, y, 0.0f);
}


void imVertex3(union Vector3 position) {
    imVertex3f(position.x, position.y, position.z);
}


void imVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    MODE_MUST_BE(COMMAND_PRIMITIVE);

    current_vertex.position.x = x;
    current_vertex.position.y = y;
    current_vertex.position.z = z;

    if (vertex_count < VERTEX_MAX_COUNT) {
        current_command.primitive.count++;
        vertices[vertex_count++] = current_vertex;
    }
}


static void set_matrix(union Matrix4 matrix, int offset) {
    glBindBuffer(GL_UNIFORM_BUFFER, MATRICES.id); {
        glBufferSubData(GL_UNIFORM_BUFFER,
                        offset * sizeof(union Matrix4),
                        sizeof(union Matrix4),
                        &matrix);
    } glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


void imFlush(void) {
    rtFillBuffer();
    rtFlush();
}


GLuint64 rtLoadMesh(const char * filepath) {
    char * source = fopenstr(filepath);

    if (!source) {
	Warn("Unable to open mesh file. Does %s exist?\n", filepath);
	return 0;
    }

    rtBegin(); {
	char * line = source;
	while (line) {
	    char * endline = strchr(line, '\n');
	    if (endline) {
		*endline = '\0';

		union Vector3 position, normal;

		if (sscanf(line,
			   "%*i %f,%f,%f %f,%f,%f",
			   &position.x, &position.y, &position.z,
			   &normal.x, &normal.y, &normal.z) == 6) {
		    imNormal3(normal);
		    imVertex3(position);
		}

		line = endline + 1;
	    } else {
		line = NULL;
	    }
	}
    } GLuint64 id = rtEnd();

    free(source);

    return id;
}


GLuint64 rtGenVertexArray(void) {
    GLuint vertex_array, vertex_buffer;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array); {
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); {
            glBufferData(GL_ARRAY_BUFFER,
                         sizeof(vertices),
                         NULL,
                         GL_DYNAMIC_DRAW);

            size_t offset = 0;
            
            /* Position */
            glVertexAttribPointer(0,
                                  3,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(struct Vertex),
                                  (void * )offset);
            glEnableVertexAttribArray(0);
            offset += sizeof(union Vector3);
            
            /* Normal */
            glVertexAttribPointer(1,
                                  3,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(struct Vertex),
                                  (void * )offset);
            glEnableVertexAttribArray(1);
            offset += sizeof(union Vector3);
            
            /* Color */
            glVertexAttribPointer(2,
                                  4,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(struct Vertex),
                                  (void * )offset);
            glEnableVertexAttribArray(2);
            offset += sizeof(union Vector4);
            
            /* Texture Coordinates */
            glVertexAttribPointer(3,
                                  2,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(struct Vertex),
                                  (void * )offset);
            glEnableVertexAttribArray(3);
            offset += sizeof(union Vector2);
        }
    } /* glBindVertexArray(0); */

    glLogErrors();

    return ((GLuint64)vertex_array << 32) | (GLuint64)vertex_buffer;
}


void rtBindVertexArray(GLuint64 id) {
    glBindVertexArray((GLuint)(id >> 32));
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)id);
}


void rtDeleteVertexArray(GLuint64 id) {
    GLuint vertex_buffer = (GLuint)id;
    GLuint vertex_array = (GLuint)(id >> 32);
    glDeleteBuffers(1, &vertex_buffer);
    glDeleteVertexArrays(1, &vertex_array);
}


static GLint rtBegin_vertex_count;


void rtBegin(void) {
    MODE_MUST_BE(COMMAND_ANY);
    current_mode = COMMAND_PRIMITIVE;
    rtBegin_vertex_count = vertex_count;
}


GLuint64 rtEnd(void) {
    static GLsizei previous_vertex_count = 0;

    MODE_MUST_BE_OR_ERR(COMMAND_PRIMITIVE, 0);
    current_mode = COMMAND_ANY;

    
    GLsizei vertices_added = vertex_count - previous_vertex_count;
    previous_vertex_count = vertex_count;
    return ((GLuint64)rtBegin_vertex_count << 32) | (GLuint64)vertices_added;
}


void rtDrawArrays(GLenum mode, GLuint64 first_count) {
    MODE_MUST_BE(COMMAND_ANY);

    current_command.type = COMMAND_PRIMITIVE;
    current_command.primitive.mode = mode;
    current_command.primitive.first = (GLint)(first_count >> 32);
    current_command.primitive.count = (GLsizei)first_count;

    ADVANCE_COMMAND();
}


void rtDrawArraysInstanced(GLenum mode, GLuint64 first_count, GLsizei instancecount) {
    MODE_MUST_BE(COMMAND_ANY);

    current_command.type = COMMAND_INSTANCED_PRIMITIVE;
    current_command.primitive.mode = mode;
    current_command.primitive.first = (GLint)(first_count >> 32);
    current_command.primitive.count = (GLsizei)first_count;
    current_command.primitive.instancecount = instancecount;

    ADVANCE_COMMAND();
}


void rtFillBuffer(void) {
    glLogErrors();

    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    vertex_count * sizeof(struct Vertex),
                    &vertices);

    glLogErrors();
}


void rtFlush(void) {
    MODE_MUST_BE(COMMAND_ANY);

    glLogErrors();
    
    GLint i;
    for (i=0; i<command_count; ++i) {
        struct Command command = commands[i];

        switch (command.type) {
        case COMMAND_ACTIVE_TEXTURE:
            glLogErrors();
            glActiveTexture(command.active_texture.texture);
            glLogErrors();
        case COMMAND_ANY:
            break;
        case COMMAND_INSTANCED_PRIMITIVE:
            glLogErrors();
            glDrawArraysInstanced(command.primitive.mode,
                                  command.primitive.first,
                                  command.primitive.count,
                                  command.primitive.instancecount);
            glLogErrors();
            break;
        case COMMAND_MODEL:
            glLogErrors();
            set_matrix(command.model, 2);
            glLogErrors();
            break;
        case COMMAND_PRIMITIVE:
            glLogErrors();
            glDrawArrays(command.primitive.mode,
                         command.primitive.first,
                         command.primitive.count);
            glLogErrors();
            break;
        case COMMAND_PROGRAM:
            glUseProgram(command.program.id);
            glLogErrors();
            break;
        case COMMAND_PROJECTION:
            set_matrix(command.projection, 0);
            glLogErrors();
            break;
        case COMMAND_BIND_TEXTURE:
            glBindTexture(command.bind_texture.target, command.bind_texture.id);
            glLogErrors();
            break;
        case COMMAND_VIEW:
            set_matrix(command.view, 1);
            glLogErrors();
            break;
        }
    }

    glLogErrors();
    
    command_count = 0;
    vertex_count = 0;
    /* TODO It might be worth resetting the current vertex to a blank state */
}


GLuint LoadShader(GLenum type, const char * filepath) {
    char * source = fopenstr(filepath);
    
    if (!source) {
        Err("Unable to open shader source file. Does %s exist?\n",
               filepath);
        return 0;
    }
    
    GLuint id = glShaderFromSource(type, source);
    free(source);

    /* Check for errors after all of those OpenGL calls */
    glLogErrors();
    
    return id;
}


GLuint LoadProgram(GLuint vertex, GLuint fragment) {
    GLuint id = glProgramFromShaders(vertex, fragment);

    glLogErrors();

    GLuint matrix_index = glGetUniformBlockIndex(id,
                                                 MATRICES.name);

    glLogErrors();

    glUniformBlockBinding(id,
                          matrix_index,
                          MATRICES.bind);

    /* Check for errors after all of those OpenGL calls */
    glLogErrors();

    return id;
}
