#ifndef __LOGO_H__
#define __LOGO_H__

#include "widget.h"
#include "mesh.h"

/* Rotating Logo (3D-mesh) */
class Logo: public EGLWidget {
private:
    /* Rotation angle */
    GLfloat angle;
    /* Shader parameter descriptor */
    GLint attr_pos;

    /* Vertex buffer */
    GLuint vertex_buf;
    /* Index buffer (triangles) */
    GLuint triangles_buf;

    /* Number of vertexes */
    GLsizei vertex_num;
    /* Number of triangles */
    GLsizei triangles_num;

    /* Scale factor */
    GLfloat mesh_scale;
public:
    Logo(const char* file, float scale);

    virtual void prepare();
    virtual void draw();
    virtual const char* vertexShader();
    virtual const char* fragmentShader();
};


#endif
