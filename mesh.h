#ifndef __MESH_H__
#define __MESH_H__

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <vector>

/* Simple 3D mesh */
class Mesh {
private:
    /* Vertex */
    typedef struct {
        GLfloat x;
        GLfloat y;
        GLfloat z;
    } vertex_t;

    /* Triangle */
    typedef struct {
        GLushort a;
        GLushort b;
        GLushort c;
    } triangle_t;

    /* Buffers to store 3d-data */
    std::vector<vertex_t> vertices;
    std::vector<triangle_t> triangles;

public:
    Mesh() {}
    virtual ~Mesh() {}

    /* Load mesh from a file */
    void load(const char* file);

    /* Load vertex data into EGL buffer */
    GLuint genVertexBuffer();
    /* Load index (triangle) data into EGL buffer */
    GLuint genTrianglesBuffer();

    /* Number of vertices/triangles */
    GLsizei getVertexNum();
    GLsizei getTrianglesNum();
};


#endif
