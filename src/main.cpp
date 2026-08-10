#define SDL_MAIN_USE_CALLBACKS 1 // Use the callbacks instead of main()

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cglm/cglm.h>

#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif // __EMSCRIPTEN__

#include "file_utils.h"
#include "line_drawer_2d.h"
#include "shader_program.h"

typedef struct
{
    SDL_Window *window;
    SDL_GLContext glContext;
    GLuint shaderProgram;
    LineDrawer2D *lineDrawer;
    bool projectionNeedsUpdate;
    mat4 projView2D;
} App;

void updateProjection(void *appState, int width, int height)
{
    App *app = (App *)appState;
    float windowAspect = (float)width / (float)height;

    // Update OpenGL viewport to match the new pixel dimensions
    glViewport(0, 0, width, height);

    // Use orthographic projection for 2D rendering.
    // Adjust bounds so (0,0) stays at the center and scales with aspect ratio.
    float orthoSize = 100.0f; // Half-height of the view world units

    float left, right, bottom, top;

    if (width >= height)
    {
        // Landscape mode: fix vertical height, scale width by aspect ratio
        left = -orthoSize * windowAspect;
        right = orthoSize * windowAspect;
        bottom = -orthoSize;
        top = orthoSize;
    }
    else
    {
        // Portrait mode: fix horizontal width, scale height by inverse aspect ratio
        left = -orthoSize;
        right = orthoSize;
        bottom = -orthoSize / windowAspect;
        top = orthoSize / windowAspect;
    }

    mat4 projMatrix;
    glm_ortho(left, right, bottom, top, -1.0f, 1.0f, projMatrix);

    // Identity view matrix for standard 2D view (looking down -Z axis from origin)
    mat4 viewMatrix;
    vec3 eye = { 0.0f, 0.0f, 1.0f };
    vec3 center = { 0.0f, 0.0f, 0.0f };
    vec3 up = { 0.0f, 1.0f, 0.0f };
    glm_lookat(eye, center, up, viewMatrix);

    mat4 projViewMatrix;
    glm_mat4_mul(projMatrix, viewMatrix, projViewMatrix);

    LineDrawer2D_setProjViewMatrix(app->lineDrawer, projViewMatrix);
}

// This function runs once at startup
SDL_AppResult SDL_AppInit(void **appState, int argc, char *argv[])
{
    App *app = (App *)SDL_malloc(sizeof(App));
    *appState = app;

#ifndef __EMSCRIPTEN__
    if (!SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "60"))
    {
        SDL_Log("Failed to set a frame rate: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
#endif

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1); // Enable MULTISAMPLE
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8); // Can be 2, 4, 8 or 16

#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
    // Mobile and Web: Request OpenGL ES 3.0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    // Windows/Desktop: Request OpenGL 3.3 Core Profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // Explicitly ask for forward compatibility for better driver support
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    int w = 480; // Default width for Windows
    int h = 480; // Default height for Windows

#if ANDROID
    flags |= SDL_WINDOW_FULLSCREEN;
    w = 0;
    h = 0;
#endif // ANDROID

    app->window = SDL_CreateWindow("SDL3, OpenGL", w, h, flags);
    if (!app->window)
    {
        SDL_Log("Couldn't create the window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    app->glContext = SDL_GL_CreateContext(app->window);
    if (!app->glContext)
    {
        SDL_Log("Couldn't create the glContext: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

#ifdef WIN32
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        SDL_Log("Failed to initialize OpenGL function pointers");
        return SDL_APP_FAILURE;
    }
#endif // WIN32

    // Disable depth testing for pure 2D rendering
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 1.f);

    // Load shader sources
#if defined(__ANDROID__)
    char *vertexSource = readFile("shaders/shader.vert");
    char *fragmentSource = readFile("shaders/shader.frag");
#else
    char *vertexSource = readFile("assets/shaders/shader.vert");
    char *fragmentSource = readFile("assets/shaders/shader.frag");
#endif

    if (vertexSource != NULL && fragmentSource != NULL)
    {
        app->shaderProgram = createShaderProgram(vertexSource, fragmentSource);
        if (!app->shaderProgram)
        {
            return SDL_APP_FAILURE;
        }

        SDL_free(vertexSource);
        SDL_free(fragmentSource);
    }
    else
    {
        if (vertexSource)
            SDL_free(vertexSource);
        if (fragmentSource)
            SDL_free(fragmentSource);

        SDL_Log("Error: Could not load shader files.");
        return SDL_APP_FAILURE;
    }

    app->lineDrawer = LineDrawer2D_getInstance();
    mat4 dummyMatrix = GLM_MAT4_IDENTITY_INIT;
    LineDrawer2D_init(app->lineDrawer, app->shaderProgram, dummyMatrix);

    return SDL_APP_CONTINUE;
}

// This function runs when a new event occurs
SDL_AppResult SDL_AppEvent(void *appState, SDL_Event *event)
{
    App *app = (App *)appState;

    switch (event->type)
    {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        default:
            break;
    }

    return SDL_APP_CONTINUE;
}

// Main rendering loop
SDL_AppResult SDL_AppIterate(void *appState)
{
    App *app = (App *)appState;
    int winW, winH;
    SDL_GetWindowSizeInPixels(app->window, &winW, &winH);

    // Update 2D orthographic projection matrix
    updateProjection(app, winW, winH);

    glViewport(0, 0, winW, winH);

    glClear(GL_COLOR_BUFFER_BIT);

    // First line
    vec3 start1 = { -60.f, 50.f, 0.f };
    vec3 end1 = { 40.f, 80.f, 0.f };
    vec3 color1 = { 1.f, 0.5f, 0.5f };
    LineDrawer2D_draw(app->lineDrawer, start1, end1, color1, 1.f);

    // Second line
    vec3 start2 = { -28.f, 37.f, 0.f };
    vec3 end2 = { 80.f, 25.f, 0.f };
    vec3 color2 = { 0.5f, 0.5f, 1.f };
    LineDrawer2D_draw(app->lineDrawer, start2, end2, color2, 3.f);

    // Polygonal chain
    vec3 polyColor = { 0.5f, 1.f, 0.5f };
    float polyThickness = 5.f;

    vec3 p0 = { -81.f, -73.f, 0.f };
    vec3 p1 = { -43.f, -20.f, 0.f };
    vec3 p2 = { 0.f, -5.f, 0.f };
    vec3 p3 = { 25.f, -10.f, 0.f };
    vec3 p4 = { 52.f, -70.f, 0.f };
    vec3 p5 = { 77.f, -5.f, 0.f };

    LineDrawer2D_draw(app->lineDrawer, p0, p1, polyColor, polyThickness);
    LineDrawer2D_draw(app->lineDrawer, p1, p2, polyColor, polyThickness);
    LineDrawer2D_draw(app->lineDrawer, p2, p3, polyColor, polyThickness);
    LineDrawer2D_draw(app->lineDrawer, p3, p4, polyColor, polyThickness);
    LineDrawer2D_draw(app->lineDrawer, p4, p5, polyColor, polyThickness);

    // Axis helper: X-axis (Red), Y-axis (Green)
    float axisLength = 80.0f;
    vec3 origin = { 0.f, 0.f, 0.f };

    // X-axis
    vec3 xAxis = { axisLength, 0.f, 0.f };
    vec3 xColor = { 1.f, 0.f, 0.f };
    LineDrawer2D_draw(app->lineDrawer, origin, xAxis, xColor, 1.f);

    // Y-axis
    vec3 yAxis = { 0.f, axisLength, 0.f };
    vec3 yColor = { 0.f, 1.f, 0.f };
    LineDrawer2D_draw(app->lineDrawer, origin, yAxis, yColor, 1.f);

    // Swap buffers
    SDL_GL_SwapWindow(app->window);
    return SDL_APP_CONTINUE;
}

// Cleanup function
void SDL_AppQuit(void *appState, SDL_AppResult result)
{
    App *app = (App *)appState;
    if (app)
    {
        glDeleteProgram(app->shaderProgram);
        LineDrawer2D_cleanup(app->lineDrawer);
        SDL_GL_DestroyContext(app->glContext);
        SDL_DestroyWindow(app->window);
        SDL_free(app);
    }
    SDL_Quit();
}
