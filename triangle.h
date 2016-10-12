#ifndef __TRIANGLE_H__
#define __TRIANGLE_H__

#include "widget.h"

/* Rotating triangle widget */
class Triangle: public EGLWidget {
private:
    /* Rotation angle */
    GLfloat angle;
    /* Shader parameters */
    GLint attr_pos;
    GLint attr_color;
public:
    Triangle();
    virtual void prepare();
    virtual void draw();
    virtual const char* vertexShader();
    virtual const char* fragmentShader();
};

#endif
