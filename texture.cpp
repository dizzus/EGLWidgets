#include <string.h>
#include <stdio.h>

#include "texture.h"
#include "pngloader.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

/* Vertex shader file */
const char* Texture::vertexShader() {
    return "shaders/texture_vertex.shader";
}

/* Pixel shader file */
const char* Texture::fragmentShader() {
    return "shaders/texture_fragment.shader";
}

/* Plane made of two triangles */
/*
  B +----------+ A          x: +1           t: 1
    |         /|            |                |
    |       /  |            |                |
    |     /    |  y: -1-----+----- y: +1     |
    |   /      |            |                |
    | /        |            |                +----------- s: 1
  C +----------+ D         x: -1             0
*/

/* Vertex coordinates */
static const GLfloat verts[] = {
    +1, +1, 0, 1, 1,                /* A (x: +1, y: +1, z: 0, t: 1, s: 1) */
    -1, +1, 0, 0, 1,                /* B (x: -1, y: +1, z: 0, t: 0, s: 1) */
    -1, -1, 0, 0, 0,                /* C (x: -1, y: -1, z: 0, t: 0, s: 0) */
    +1, -1, 0, 1, 0,                /* D (x: +1, y: -1, z: 0, t: 1, s: 0) */
};

/* Index coordinates (triangles) */
static const GLushort indexes[] = {
    0, 1, 2,                        /* A -> B -> C */
    0, 2, 3,                        /* A -> C -> D */
};

/* Initialization before the main loop */
void Texture::prepare() {
    /* Call parent */
    EGLWidget::prepare();

    /* Enable transparency */
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Load vertexes */
    GLuint vert_buf;
    glGenBuffers(1, &vert_buf);
    glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    /* Create shader parameter 'vertex_xyz' which represents a vertex */
    v_xyz = glGetAttribLocation(program, "vertex_xyz");
    glEnableVertexAttribArray(v_xyz);
    /* Vertex structure: 3 float-s per coordinate, total 5 floats, coordinate data starts at index 0 */
    glVertexAttribPointer(v_xyz, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);

    /* Prepare shader parameter 'vertex"st' which represent texture coordinates */
    v_st  = glGetAttribLocation(program, "vertex_st");
    glEnableVertexAttribArray(v_st);
    /* Texture structure: 2 floats per texture coordinate, total 5 floats, texture data starts at index 3 */
    glVertexAttribPointer(v_st, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    /* Load our triangle into the buffer */
    GLuint index_buf;
    glGenBuffers(1, &index_buf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes), indexes, GL_STATIC_DRAW);

    /* Load PNG-file as a texture */
    glEnable (GL_TEXTURE_2D);
    texture_id = load_png_as_texture(file_name);

    /* Create shader parameter which represent our texture */
    u_texture = glGetUniformLocation(program, "u_texture");
    printf("Texture id: %d, u_texture: %d\n", texture_id, u_texture);
}

/* Draw one frame */
void Texture::draw() {
    /* Call parent */
    EGLWidget::draw();

    /* Rotate our plane */
    angle += 0.01;

    /* Calculate MVP matrix */
    mat4 model;
    mat4 rotated = rotate(model, angle, vec3(0, 0, -1));
    mat4 scaled = scale(rotated, vec3(0.75, 0.75, 0.75));
    mat4 mvp = rotated * scaled;

    /* Pass matrix to our shader */
    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

    /* Draw triangles */
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

/* Initialization */
Texture::Texture(const char* file): EGLWidget(0, 900, 180, 180) {
    angle = 0.0;
    v_xyz = 0;
    v_st = 0;
    u_texture = -1;
    file_name = file;
}

int main(int argc, char** argv) {
#ifdef IS_RPI
    bcm_host_init();
#endif
    try {
        /* Command line parameter: path to texture file */
        Texture texture(argc > 1 ? argv[1] : "textures/texture256x256.png");
        texture.run(20);
    } catch (const std::exception& ex) {
        fprintf(stderr, "Error: %s", ex.what());
    }
}
