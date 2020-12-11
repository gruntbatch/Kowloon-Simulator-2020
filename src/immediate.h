#pragma once


#include "GL_plus.h"
#include "mathematics.h"


/* TODO Delete this? */
extern const struct UniformBuffer MATRIX_BUFFER;


void imInitTransformBuffer(void);
void imInitInternalVertexArray(void);


void imBindVertexArray(void);


void imClear(GLbitfield mask);


void imModel(union Matrix4 model);
void imView(union Matrix4 view);
void imProjection(union Matrix4 projection);


void imActiveTexture(GLenum texture);
void imBindTexture(GLenum target, GLuint texture);
void imUseProgram(GLuint program);


void imSetLights(void* data);


void imBegin(GLenum mode);
void imEnd(void);


void imColor(union Vector4 color);
void imColor3f(GLfloat r, GLfloat g, GLfloat b);
void imColor3ub(GLubyte r, GLubyte g, GLubyte b);
void imColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);


void imNormal3(union Vector3 normal);
void imNormal3f(GLfloat x, GLfloat y, GLfloat z);


void imTexCoord(union Vector2 texcoord);
void imTexCoord2f (GLfloat u, GLfloat v);


void imVertex2(union Vector2 position);
void imVertex2f(GLfloat x, GLfloat y);
void imVertex3(union Vector3 position);
void imVertex3f(GLfloat x, GLfloat y, GLfloat z);


void imFlush(void);


GLuint LoadShader(GLenum type, const char* filepath);
GLuint LoadProgram(const char* vertex_filepath, const char* fragment_filepath);
GLuint LoadTexture(const char* filepath);
