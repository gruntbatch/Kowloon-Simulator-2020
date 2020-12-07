#pragma once


#include "GL_plus.h"
#include "mathematics.h"


struct Framebuffer {
    union Vector2 resolution;
    GLuint color;
    GLuint depth;
    GLuint buffer;
};


struct Framebuffer CreateFramebuffer(int x, int y);
