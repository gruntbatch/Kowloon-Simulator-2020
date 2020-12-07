#include "framebuffer.h"


#include "logger.h"


struct Framebuffer CreateFramebuffer(int x, int y) {
    struct Framebuffer framebuffer = { { .x=x, .y=y } };

    glGenTextures(2, &framebuffer.color);
    glGenFramebuffers(1, &framebuffer.buffer);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.buffer); {
	glBindTexture(GL_TEXTURE_2D, framebuffer.color); {
            glTexImage2D(GL_TEXTURE_2D,
                         0, GL_RGB,
                         framebuffer.resolution.x, framebuffer.resolution.y,
                         0, GL_RGB,
                         GL_UNSIGNED_BYTE,
                         NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } glBindTexture(GL_TEXTURE_2D, 0);

        glBindTexture(GL_TEXTURE_2D, framebuffer.depth); {
            glTexImage2D(GL_TEXTURE_2D,
                         0, GL_DEPTH24_STENCIL8,
                         framebuffer.resolution.x, framebuffer.resolution.y,
                         0,
                         GL_DEPTH_STENCIL,
                         GL_UNSIGNED_INT_24_8,
                         NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);           
        } glBindTexture(GL_TEXTURE_2D, 0);

        glLogErrors();

        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D,
                               framebuffer.color,
                               0);

        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_DEPTH_STENCIL_ATTACHMENT,
                               GL_TEXTURE_2D,
                               framebuffer.depth,
                               0);

	GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
            Warn("Unable to complete framebuffer because %d\n", framebuffer_status);
        }
    } glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return framebuffer;
}
