#include "GL_plus.h"


#include <assert.h>
#include <stdio.h>


const char * glEnumToString(GLenum e) {
    switch (e) {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW:
        return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW:
        return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    case 0x8031: /* not core */
        return "GL_TABLE_TOO_LARGE_EXT";
    case 0x8065: /* not core */
        return "GL_TEXTURE_TOO_LARGE_EXT";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    default:
        assert(!"Unhandled GL error code");
        return NULL;
    }
}


int glLogErrorsAtLocation(const char * f, int l) {
    GLenum e;
    int count = 0;
    while ((e = glGetError()) != GL_NO_ERROR) {
        count++;
#ifdef DEBUG
        printf("%s : %d : glGetError() = %s\n",
               f,
               l,
               glEnumToString(e));
#endif
    }
    return count;
}


int glLogErrorsAtLocationAndAssert(const char * f, int l) {
    int result = glLogErrorsAtLocation(f, l);
    assert(result == 0);
    return result;
}


void glClearColor3ub(GLubyte r, GLubyte g, GLubyte b) {
    glClearColor((float)r / 255.0, (float)g / 255.0, (float)b / 255.0, 1.0);
}


GLuint glShaderFromSource(GLenum type, const char * source) {
    GLuint id = glCreateShader(type);

    glShaderSource(id, 1, &source, NULL);

    glCompileShader(id);

    int success;
    char log[512];

    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        glGetShaderInfoLog(id, 512, NULL, log);
        printf("Error compiling shader: %s\n",
               log);
    }

    return id;
}


GLuint glProgramFromShaders(GLuint vertex, GLuint fragment) {
    GLuint program = glCreateProgram();
    
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    int success;
    char log[512];

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, log);
        printf("Error linking program: %s\n",
               log);
    }
    
    return program;
}


void glBindUniformBlock(GLuint program,
                        const GLchar * uniformBlockName,
                        GLuint uniformBlockBinding) {
    GLuint index = glGetUniformBlockIndex(program, uniformBlockName);

    if (index == GL_INVALID_INDEX) {
        assert(0);
    } else {
        glUniformBlockBinding(program, index, uniformBlockBinding);
    }
}
