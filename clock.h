#ifndef __CLOCK_H__
#define __CLOCK_H__

#include "widget.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <time.h>
#include <sys/select.h>

/* System time widget */
class Clock: public EGLWidget {
private:
    /* FreeType library resources */
    FT_Library library;
    FT_Face face;

    typedef struct {
        unsigned char r, g, b, a;
    } pixel_t;

    /* Clock texture, 256x256 size */
    pixel_t clock_texture[ 256 * 256 ];

    /* Widget rotation angle */
    GLfloat angle;

    /* Shader parameters descriptors */
    GLint v_xyz;
    GLint v_st;
    GLuint texture_id;
    GLuint u_texture;

    /* Last update time, font size */
    time_t last_time;
    int font_size;

    void printText(const char* text, int pen_x, int pen_y, int size);

public:
    Clock(const char* font, int size);
    ~Clock();

    virtual void prepare();
    virtual void draw();
    virtual const char* vertexShader();
    virtual const char* fragmentShader();
};

#endif
