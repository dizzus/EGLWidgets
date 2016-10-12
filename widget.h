#ifndef __EGLWIDGET_H__
#define __EGLWIDGET_H__

#include <math.h>
#include <string>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#ifdef IS_RPI
#   include <bcm_host.h>
#else
#   include <X11/Xlib.h>
#   include <X11/Xutil.h>
#   include <X11/Xproto.h>
#   include <X11/Xatom.h>
#endif

/* Base class of a EGL widget */
class EGLWidget {
private:
    /* Native window descriptor */
#ifdef IS_RPI
    EGL_DISPMANX_WINDOW_T nativeWindow;
#else
    Window nativeWindow;
#endif
    /* Frame counter */
    GLfloat frames;
    /* Frame counter shader descriptor */
    GLint u_frames;

    void init();
    void createSurface(int sx, int sy, int sw, int sh);
    void finish();

    std::string loadFile(const char* file);
    void loadShaders();
    GLuint compileShader(GLenum type, const char* file);
    void setShaderParameters();

protected:
    /* MVP shader descriptor*/
    GLint u_mvp;

    /* Widget coordinates */
    uint32_t x;
    uint32_t y;

    /* Widget size */
    uint32_t width;
    uint32_t height;

    /* Shader program descriptor */
    GLuint program;

    /* EGL specific descriptors */
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;

protected:
    /* Virtual functions to be overloaded in the subclass: */

    /* Called before main loop. Initialize your widget here */
    virtual void prepare();
    /* Called in the main loop to draw one frame */
    virtual void draw();
    /* Called to get path to your widget's vertex shader file */
    virtual const char* vertexShader();
    /* Called to get path to your widget's pixel shader file */
    virtual const char* fragmentShader();

public:
    EGLWidget(int sx, int sy, int sw, int sh) {
        init();
        createSurface(sx, sy, sw, sh);
    }

    virtual ~EGLWidget() {
        finish();
    }

    uint32_t getWidth()  { return width; }
    uint32_t getHeight() { return height; }

    /* Run main loop at given FPS */
    void run(int fps);
};

#endif
