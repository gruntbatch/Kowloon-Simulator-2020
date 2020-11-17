#pragma once


#include "GL_plus.h"
#include "mathematics.h"


/* GLuint64 rtLoadMesh(const char * filepath); */
/* void rtDrawMesh(GLuint64 id); */


GLuint64 rtGenVertexArray(void);
void rtBindVertexArray(GLuint64 id);
void rtDeleteVertexArray(GLuint64 id);


void rtBegin(void);
GLuint64 rtEnd(void);


void rtDrawArrays(GLenum mode, GLuint64 first_count);
void rtDrawArraysInstanced(GLenum mode, GLuint64 first_count, GLsizei instancecount);


void rtFillBuffer(void);


void rtFlush(void);
