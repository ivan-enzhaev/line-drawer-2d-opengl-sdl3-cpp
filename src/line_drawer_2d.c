#include "line_drawer_2d.h"

#include <SDL3/SDL.h>
#include <math.h>

static LineDrawer2D *instance = NULL;

// Constructor-like initialization
void LineDrawer2D_init(LineDrawer2D *self, GLuint shaderProgram, mat4 projViewMatrix)
{
    if (self)
    {
        self->shaderProgram = shaderProgram;
        glm_mat4_copy(projViewMatrix, self->projViewMatrix);

        // Set up vertex data and buffers
        // clang-format off
        float vertices[] = {
            -0.5f, -0.5f, 0.f,
            -0.5f, 0.5f, 0.f,
            0.5f, -0.5f, 0.f,
            0.5f, 0.5f, 0.f
        };
        // clang-format on
        self->vertCount = sizeof(vertices) / (sizeof(float) * 3);

        // Generate and bind VAO
        glGenVertexArrays(1, &self->vao);
        glBindVertexArray(self->vao);

        // Generate and bind VBO
        glGenBuffers(1, &self->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, self->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Setup Layout 0
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

        // Unbind to prevent accidental state changes
        glBindVertexArray(0);

        // Activate the current shader program to access shader variables
        glUseProgram(self->shaderProgram);

        // Uniforms
        self->uColorLocation = glGetUniformLocation(shaderProgram, "uColor");
        self->uMvpLocation = glGetUniformLocation(shaderProgram, "uMvpMatrix");
    }
}

LineDrawer2D *LineDrawer2D_getInstance(void)
{
    if (instance == NULL)
    {
        instance = malloc(sizeof(LineDrawer2D));
    }
    return instance;
}

void LineDrawer2D_setProjViewMatrix(LineDrawer2D *self, mat4 projViewMatrix)
{
    glm_mat4_copy(projViewMatrix, self->projViewMatrix);
}

void LineDrawer2D_draw(LineDrawer2D *self, vec3 start, vec3 end, vec3 color, float thickness)
{
    if (!self)
        return;

    // Find the vector and the center of the segment
    vec3 v;
    glm_vec3_sub(end, start, v);

    // center = start + v / 2
    vec3 v_half, center;
    glm_vec3_scale(v, 0.5f, v_half);
    glm_vec3_add(start, v_half, center);

    // Find the length of the segment
    float length = glm_vec3_norm(v);

    // Guard against zero-length vectors to avoid NaN matrices
    if (length < 0.0001f)
    {
        return;
    }

    // Calculate the angle in radians using atan2(y, x)
    // This gives the angle between the X-axis and the vector 'v'
    float angle = atan2f(v[1], v[0]);

    // Create the Model Matrix
    // Start with identity
    glm_mat4_identity(self->modelMatrix);

    // Translate to the center of the line
    glm_translate(self->modelMatrix, center);

    // Rotate around the Z-axis (0, 0, 1)
    vec3 z_axis = { 0.0f, 0.0f, 1.0f };
    glm_rotate(self->modelMatrix, angle, z_axis);

    // Scale the unit square
    // The X-scale is the length of the line, Y-scale is the thickness
    glm_scale(self->modelMatrix, (vec3) { length, thickness, 1.0f });

    // Combine projView matrix and model matrix into one MVP matrix
    glm_mat4_mul(self->projViewMatrix, self->modelMatrix, self->mvpMatrix);

    glBindVertexArray(self->vao);

    glUseProgram(self->shaderProgram);

    // Send MVP matrix to the vertex shader
    glUniformMatrix4fv(self->uMvpLocation, 1, GL_FALSE, self->mvpMatrix[0]);

    // Send color value to fragment shader
    glUniform3fv(self->uColorLocation, 1, &color[0]);

    // Draw the triangle strip (4 vertices)
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Clean up binding
    glBindVertexArray(0);

    // // Compute vector v = end - start
    // vec3 v;
    // glm_vec3_sub(end, start, v);

    // // centerPosition = start + v / 2
    // vec3 center;
    // glm_vec3_scale(v, 0.5f, center);
    // glm_vec3_add(start, center, center);

    // // length = ||v||
    // float length = glm_vec3_norm(v);
    // vec3 norm;
    // if (length > 1e-8f)
    // {
    //     glm_vec3_normalize_to(v, norm);
    // }
    // else
    // {
    //     // Degenerate segment: point; pick X axis
    //     norm[0] = 1.0f;
    //     norm[1] = 0.0f;
    //     norm[2] = 0.0f;
    //     length = 0.0f;
    // }

    // versor rotation;
    // vec3 from = { 1.0f, 0.0f, 0.0f };
    // MathHelper_rotationTo(from, norm, rotation);

    // // Build model matrix
    // mat4 model;
    // glm_mat4_identity(model);
    // glm_translate(model, center);

    // mat4 rotMat;
    // glm_quat_mat4(rotation, rotMat);
    // glm_mat4_mul(model, rotMat, model);

    // mat4 scaleMat = GLM_MAT4_IDENTITY_INIT;
    // scaleMat[0][0] = length;
    // scaleMat[1][1] = thickness;
    // scaleMat[2][2] = thickness;
    // glm_mat4_mul(model, scaleMat, model);

    // mat4 mvp;
    // glm_mat4_mul(self->projViewMatrix, model, mvp);

    // // Upload uniforms
    // glUseProgram(self->shaderProgram);
    // glUniformMatrix4fv(self->uMvpLocation, 1, GL_FALSE, (const GLfloat *)mvp);
    // glUniform3fv(self->uColorLocation, 1, (const GLfloat *)color);

    // // Bind VAO and Draw
    // glBindVertexArray(self->vao);
    // glDrawArrays(GL_TRIANGLE_STRIP, 0, self->vertCount);

    // // Clean up binding
    // glBindVertexArray(0);
}

void LineDrawer2D_cleanup(LineDrawer2D *self)
{
    if (!self)
        return;

    glDeleteVertexArrays(1, &self->vao);
    glDeleteBuffers(1, &self->vbo);

    free(instance);
    instance = NULL;
}
