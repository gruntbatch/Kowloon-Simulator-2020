#pragma once


#include <GL/glew.h>


const char * glEnumToString(GLenum e);
int glLogErrorsAtLocation(const char * file, int line);
int glLogErrorsAtLocationAndAssert(const char * file, int line);


#ifdef STRICT
#define glLogErrors() glLogErrorsAtLocationAndAssert(__FILE__, __LINE__)
#else
#define glLogErrors() glLogErrorsAtLocation(__FILE__, __LINE__)
#endif


void glClearColor3ub(GLubyte r, GLubyte g, GLubyte b);


GLuint glShaderFromSource(GLenum type, const GLchar * source);
GLuint glProgramFromShaders(GLuint vertex, GLuint fragment);


void glBindUniformBlock(GLuint program,
                        const GLchar * uniformBlockName,
                        GLuint uniformBlockBinding);


struct UniformBuffer {
    const GLchar * name;
    const GLuint size;
    const GLuint bind;
    GLuint id;
};
