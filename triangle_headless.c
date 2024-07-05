#include <fcntl.h>
#include <gbm.h>
#include <unistd.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to save the rendered image to a file
void save_image(const char *filename, int width, int height) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Failed to open file for writing");
        exit(EXIT_FAILURE);
    }

    // Allocate buffer to read the pixels
    GLubyte *pixels = malloc(width * height * 4);
    if (!pixels) {
        perror("Failed to allocate pixel buffer");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    // Read the pixels from the framebuffer
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Write the PPM header
    fprintf(fp, "P6\n%d %d\n255\n", width, height);

    // Write pixel data to file
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            GLubyte *pixel = pixels + (y * width + x) * 4;
            fwrite(pixel, 1, 3, fp);  // Only write RGB, skip A
        }
    }

    fclose(fp);
    free(pixels);
}

// Identifiers for the GL objects
GLuint VAO, VBO, shaderProgram;

// Vertex Shader
static const char* vShader = "                                                  \n\
#version 300 es                                                                 \n\
                                                                                \n\
layout (location = 0) in vec3 position;                                         \n\
                                                                                \n\
void main()                                                                     \n\
{                                                                               \n\
    gl_Position = vec4(0.4 * position.x, 0.4 * position.y, position.z, 1.0f);   \n\
}";

// Fragment Shader
static const char* fShader = "              \n\
#version 300 es                             \n\
precision mediump float;                    \n\
out vec4 color;                             \n\
                                            \n\
void main()                                 \n\
{                                           \n\
    color = vec4(1.0f, 0.0f, 0.0f, 1.0f);   \n\
}";

void CreateTriangle()
{
    // Vertex positions for the triangle
    GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

    // Specify a VAO for our triangle object
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

        // Specify a VBO to bind to the above VAO
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

            // Loading up the data of the triangle into the VBO
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            // Setting up attribute pointer for shader access
            // Arguments: layout location, size, type, normalise?, stride, offset
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
            // Layout location
            glEnableVertexAttribArray(0);
        
        // Unbinding the VBO
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Unbinding the VAO
    glBindVertexArray(0);
}

void AddShader(GLuint program, const char* shaderSource, GLenum shaderType)
{
    // Create an empty shader object
    GLuint shader = glCreateShader(shaderType);

    // Convert the source code for the shader 
    // into the right type for loading
    const GLchar* sourceCode[1];
    sourceCode[0] = shaderSource;

    // Convert the length of the source code for 
    // the shader into the right type for loading
    GLint sourceLength[1];
    sourceLength[0] = strlen(shaderSource);

    // Load the shader source into the 
    // empty shader object and compile the code
    glShaderSource(shader, 1, sourceCode, sourceLength);
    glCompileShader(shader);

    // Find and log error if any from 
    // the above compilation process
    GLint result = 0;
    GLchar log[1024] = { 0 };

    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        printf("Error: Compilation of shader of type %d failed, '%s'\n", shaderType, log);
        return;
    }

    // Attach the shader to the shader program
    glAttachShader(program, shader);
}

void CompileShaderProgram()
{
    // Create an empty shader program object
    shaderProgram = glCreateProgram();

    // Check if it was created successfully
    if (!shaderProgram)
    {
        printf("Error: Generation of shader program failed!\n");
        return;
    }

    // Attaching our vertex and fragment shaders to the shader program
    AddShader(shaderProgram, vShader, GL_VERTEX_SHADER);
    AddShader(shaderProgram, fShader, GL_FRAGMENT_SHADER);

    // Setting up error logging objects
    GLint result = 0;
    GLchar log[1024] = { 0 };

    // Perform shader program linking
    glLinkProgram(shaderProgram);

    // Find and log errors if any from the linking process done above
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &result);
    if (!result)
    {
        glGetProgramInfoLog(shaderProgram, sizeof(log), NULL, log);
        printf("Error: Linking of the shader program failed, '%s'\n", log);
        return;
    }

    // Perform shader program validation
    glValidateProgram(shaderProgram);

    // Find and log errors if any from the validation process done above
    glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &result);
    if (!result)
    {
        glGetProgramInfoLog(shaderProgram, sizeof(log), NULL, log);
        printf("Error: Shader program validation failed, '%s'", log);
        return;
    }
}

int main() {
    const int width = 800;
    const int height = 600;

    // Open the DRM device
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        perror("Failed to open DRM device");
        return EXIT_FAILURE;
    }

    // Create a GBM device
    struct gbm_device *gbm = gbm_create_device(drm_fd);
    if (!gbm) {
        perror("Failed to create GBM device");
        close(drm_fd);
        return EXIT_FAILURE;
    }

    // Create a GBM surface
    struct gbm_surface *surface = gbm_surface_create(gbm, width, height, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
    if (!surface) {
        perror("Failed to create GBM surface");
        gbm_device_destroy(gbm);
        close(drm_fd);
        return EXIT_FAILURE;
    }

    // Get an EGL display connection
    EGLDisplay egl_display = eglGetDisplay(gbm);
    if (egl_display == EGL_NO_DISPLAY) {
        perror("Failed to get EGL display");
        gbm_surface_destroy(surface);
        gbm_device_destroy(gbm);
        close(drm_fd);
        return EXIT_FAILURE;
    }

    // Initialize the EGL display connection
    if (!eglInitialize(egl_display, NULL, NULL)) {
        perror("Failed to initialize EGL");
        eglTerminate(egl_display);
        gbm_surface_destroy(surface);
        gbm_device_destroy(gbm);
        close(drm_fd);
        return EXIT_FAILURE;
    }

    // Choose an appropriate EGL configuration
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_NONE
    };
    EGLConfig config;
    EGLint num_configs;
    if (!eglChooseConfig(egl_display, config_attribs, &config, 1, &num_configs)) {
        perror("Failed to choose EGL config");
        eglTerminate(egl_display);
        gbm_surface_destroy(surface);
        gbm_device_destroy(gbm);
        close(drm_fd);
        return EXIT_FAILURE;
    }

    // Create an EGL context
    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    EGLContext context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, context_attribs);
    if (context == EGL_NO_CONTEXT) {
        perror("Failed to create EGL context");
        eglTerminate(egl_display);
        gbm_surface_destroy(surface);
        gbm_device_destroy(gbm);
        close(drm_fd);
        return EXIT_FAILURE;
    }

    // Create an EGL window surface
    EGLSurface egl_surface = eglCreateWindowSurface(egl_display, config, (EGLNativeWindowType)surface, NULL);
    if (egl_surface == EGL_NO_SURFACE) {
        perror("Failed to create EGL surface");
        eglDestroyContext(egl_display, context);
        eglTerminate(egl_display);
        gbm_surface_destroy(surface);
        gbm_device_destroy(gbm);
        close(drm_fd);
        return EXIT_FAILURE;
    }

    // Make the context and surface current
    if (!eglMakeCurrent(egl_display, egl_surface, egl_surface, context)) {
        perror("Failed to make EGL context current");
        eglDestroySurface(egl_display, egl_surface);
        eglDestroyContext(egl_display, context);
        eglTerminate(egl_display);
        gbm_surface_destroy(surface);
        gbm_device_destroy(gbm);
        close(drm_fd);
        return EXIT_FAILURE;
    }

    // Set up the viewport
    glViewport(0, 0, width, height);

    // Generate the triangle and shaders
    CreateTriangle();
    CompileShaderProgram();

            // Clear the window
        glClearColor(0.3f, 0.5f, 0.6f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

        // Activate the required shader for drawing
        glUseProgram(shaderProgram);
            // Bind the required object's VAO
            glBindVertexArray(VAO);
                // Perform the draw call to initialise the pipeline
                // Arguments: drawing mode, offset, number of points
                glDrawArrays(GL_TRIANGLES, 0, 3);
            
            // Unbinding just for completeness
           glBindVertexArray(0);
        // Deactivating shaders for completeness
        glUseProgram(0);


    // Swap the buffers to display the rendered image
    eglSwapBuffers(egl_display, egl_surface);

    // Save the rendered image to a file
    save_image("triangle.ppm", width, height);

    // Clean up
    eglDestroySurface(egl_display, egl_surface);
    eglDestroyContext(egl_display, context);
    eglTerminate(egl_display);
    gbm_surface_destroy(surface);
    gbm_device_destroy(gbm);
    close(drm_fd);

    return EXIT_SUCCESS;
}
