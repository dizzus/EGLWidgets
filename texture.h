#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "widget.h"

/* Widget implements a rotating PNG-image */
class Texture: public EGLWidget {
private:
    /* Rotation angle */
    GLfloat angle;
    /* Shader parameters */
    GLint v_xyz;
    GLint v_st;
    GLuint u_texture;
    /* Texture descriptor */
    GLuint texture_id;
    /* PNG-file path */
    const char* file_name;

public:
    Texture(const char* file);
    virtual void prepare();
    virtual void draw();
    virtual const char* vertexShader();
    virtual const char* fragmentShader();
};

#endif
