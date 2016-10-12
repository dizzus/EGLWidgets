#include <stdio.h>
#include <string.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "logo.h"

using namespace glm;

/* Vertex shader file */
const char* Logo::vertexShader() {
    return "shaders/logo_vertex.shader";
}

/* Pixel shader file */
const char* Logo::fragmentShader() {
    return "shaders/logo_fragment.shader";
}

/* Initialization */
Logo::Logo(const char* file, float scale): EGLWidget(0, 0, 400, 400) {
    angle = 0.0;
    attr_pos = 0;

    /* Load our 3d-mesh from the .obj file */
    Mesh mesh;
    mesh.load(file);

    printf("Loaded OK\n");

    /* Load vertexes and triangles data */
    vertex_buf = mesh.genVertexBuffer();
    triangles_buf = mesh.genTrianglesBuffer();

    /* Print some statistics */
    vertex_num = mesh.getVertexNum();
    triangles_num = mesh.getTrianglesNum();

    mesh_scale = scale;

    printf("vertex buf: %d, triangles buf: %d, vertex num: %d, triangles num: %d\n", 
        vertex_buf, triangles_buf, vertex_num, triangles_num);
}

/* Prepare widget data before entering the main loop */
void Logo::prepare() {
    /* Call parent */
    EGLWidget::prepare();

    /* Create shader parameter 'pos' */
    attr_pos = glGetAttribLocation(program, "pos");
    glEnableVertexAttribArray(attr_pos);
    glVertexAttribPointer(attr_pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
}

/* Draw one frame */
void Logo::draw() {
    /* Call parent */
    EGLWidget::draw();

    /* Calculate MVP matrix */
    mat4 model;
    mat4 rotated = rotate(model, angle, vec3(1.0f, 1.0f, 1.0f));
    mat4 scaled = scale(rotated, vec3(mesh_scale, mesh_scale, mesh_scale));
    mat4 mvp = rotated * scaled;

    /* Pass MVP matrix to our shader */
    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
    /* Rotate our mesh */
    angle += 0.01;

    /* Draw the mesh */
    glDrawElements(GL_TRIANGLES, triangles_num * 3, GL_UNSIGNED_SHORT, 0);
}

int main(int argc, char** argv) {
#ifdef IS_RPI
    bcm_host_init();
#endif
    try {
        /* Command line parameters: .obj file path and scale factor */
        Logo logo(argc > 1 ? argv[1] : "meshes/logo3d.obj", argc > 2 ? atof(argv[2]) : 1.0);
        logo.run(30);
    } catch (const std::exception& ex) {
        fprintf(stderr, "Error: %s", ex.what());
    }
}
