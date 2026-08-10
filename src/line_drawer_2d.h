#ifndef LINE_DRAWER_2D_H
#define LINE_DRAWER_2D_H

#include <cglm/cglm.h>

#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif // __EMSCRIPTEN__

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        GLuint shaderProgram;
        GLuint vao, vbo;
        GLint uColorLocation;
        GLint uMvpLocation;
        mat4 projViewMatrix;
        mat4 modelMatrix;
        mat4 mvpMatrix;
        mat4 rotationMatrix;
        int vertCount;
    } LineDrawer2D;

    // Initialization (constructor-like)
    void LineDrawer2D_init(LineDrawer2D *self, GLuint shaderProgram,
        mat4 projViewMatrix);

    // Get the line drawer instance
    LineDrawer2D *LineDrawer2D_getInstance(void);

    // Setters
    void LineDrawer2D_setProjViewMatrix(LineDrawer2D *self, mat4 projViewMatrix);

    // Draw function
    void LineDrawer2D_draw(LineDrawer2D *self, vec3 start, vec3 end,
        vec3 color, float thickness);

    // Cleanup (destructor-lie)
    void LineDrawer2D_cleanup(LineDrawer2D *self);

#ifdef __cplusplus
}
#endif

#endif // LINE_DRAWER_2D_H
