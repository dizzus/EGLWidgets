#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>

#include "widget.h"

/* Widget initialization */
void EGLWidget::init() {
    x = 0;
    y = 0;
    width = 0;
    height = 0;
    frames = 0;

    u_mvp = -1;
    u_frames = -1;

    program = 0;
    display = EGL_NO_DISPLAY;
    context = EGL_NO_CONTEXT;
    surface = EGL_NO_SURFACE;
}

/* Free resources */
void EGLWidget::finish() {
    if( surface != EGL_NO_SURFACE ) {
        glClear(GL_COLOR_BUFFER_BIT);

        eglSwapBuffers(display, surface);
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        eglDestroySurface(display, surface);
        surface = EGL_NO_SURFACE;

        eglDestroyContext(display, context);
        context = EGL_NO_CONTEXT;

        eglTerminate(display);
        display = EGL_NO_DISPLAY;
    }
}

/* Create widget surface */
void EGLWidget::createSurface(int sx, int sy, int sw, int sh) {
    printf("Creating surface\n");

    /* Get default EGL display */
#ifdef IS_RPI
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#else
    Display* xdisplay = XOpenDisplay(NULL);
    if( ! xdisplay ) throw std::runtime_error("Cannot open X11 display");

    display = eglGetDisplay(xdisplay);
#endif

    if( display == EGL_NO_DISPLAY ) throw std::runtime_error("Cannot get display");

    /* Initialize EGL display */
    int major, minor;
    EGLBoolean result = eglInitialize(display, &major, &minor);
    if( result == EGL_FALSE ) throw std::runtime_error("Cannot initialize display");

    /* Framebuffer configuration: 8-bit color + alpha channel */
    static const EGLint attribute_list[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };
    EGLConfig config;
    EGLint num_config;

    /* Set configuration */
    result = eglChooseConfig(display, attribute_list, &config, 1, &num_config);
    if( result == EGL_FALSE ) throw std::runtime_error("Cannot choose config");

    /* Choose EGL API */
    result = eglBindAPI(EGL_OPENGL_ES_API);
    if( result == EGL_FALSE ) throw std::runtime_error("Cannot bind API");

    /* Create EGL context */
    static const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
    if( context == EGL_NO_CONTEXT ) throw std::runtime_error("Cannot create context");

#ifdef IS_RPI
    /* Raspberry PI specific initialization */
    /* Get screen size */
    int32_t rc = graphics_get_display_size(0 /* LCD */, &width, &height);
    if( rc < 0 ) throw std::runtime_error("Cannot get display size");

    VC_RECT_T dst_rect;

    /* Set coordinates and surface size */
    dst_rect.x = sx;
    dst_rect.y = sy;
    dst_rect.width  = sw;
    dst_rect.height = sh;

    VC_RECT_T src_rect;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width  = width << 16;
    src_rect.height = height << 16;

    /* Create window descriptor */
    DISPMANX_DISPLAY_HANDLE_T dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
    DISPMANX_UPDATE_HANDLE_T dispman_update   = vc_dispmanx_update_start( 0 );

    /* Set render options. One important parameter is 'layer' which basically sets our surface Z-order. 
       Since we want our widget to be always on top, we set it to a big number (100) */
    DISPMANX_ELEMENT_HANDLE_T dispman_element =
        vc_dispmanx_element_add(dispman_update, dispman_display,
                                100 /*layer*/, &dst_rect,
                                0 /*src*/, &src_rect,
                                DISPMANX_PROTECTION_NONE,
                                0  /*alpha*/, 0 /*clamp*/, DISPMANX_NO_ROTATE /*transform*/);

    nativeWindow.element = dispman_element;
    nativeWindow.width = width;
    nativeWindow.height = height;

    vc_dispmanx_update_submit_sync( dispman_update );
#else
    /* X Window specific initialization */
    /* Set widget size */
    width = sw;
    height = sh;

    /* We want 32-bit color mode */
    XVisualInfo vinfo;
    XMatchVisualInfo(xdisplay, DefaultScreen(xdisplay), 32, TrueColor, &vinfo);

    /* Create a transparent borderless window */
    XSetWindowAttributes attr;
    attr.colormap = XCreateColormap(xdisplay, DefaultRootWindow(xdisplay), vinfo.visual, AllocNone);
    attr.border_pixel = 0;
    attr.background_pixel = 0;
    attr.override_redirect = True;

    nativeWindow = XCreateWindow(xdisplay, DefaultRootWindow(xdisplay), sx, sy, sw, sh, 0, vinfo.depth,
                                 InputOutput, vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr);

    /* Tell window manager to put our window on top */
    Atom above = XInternAtom(xdisplay, "_NET_WM_STATE_ABOVE", False);
    XChangeProperty(xdisplay, nativeWindow, XInternAtom(xdisplay, "_NET_WM_STATE", False), XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) &above, 1);

    /* Remove window decorations */
    typedef struct {
        int flags;
        int functions;
        int decorations;
        int input_mode;
        int status;
    } MwmHints;

    MwmHints mwmhints;
    Atom prop;
    memset(&mwmhints, 0, sizeof(mwmhints));
    prop = XInternAtom(xdisplay, "_MOTIF_WM_HINTS", False);
    mwmhints.flags = 2; /* MWM_HINTS_DECORATIONS */
    mwmhints.decorations = 0;
    XChangeProperty(xdisplay, nativeWindow, prop, prop, 32,
                PropModeReplace, (unsigned char *) &mwmhints, 5 /*PROP_MWM_HINTS_ELEMENTS*/ );

    /* Place our window on the display */
    XSizeHints xhints;
    xhints.x = sx;
    xhints.y = sy;
    xhints.width = sw;
    xhints.height = sh;
    xhints.flags = USSize | USPosition;
    XSetNormalHints(xdisplay, nativeWindow, &xhints);
    XSetStandardProperties(xdisplay, nativeWindow, "EGLWidget", "EGLWidget", None, NULL, 0, &xhints);

    /* Show window */
    XCreateGC(xdisplay, nativeWindow, 0, 0);
    XMapWindow(xdisplay, nativeWindow);
#endif

#ifdef IS_RPI
#   define NATIVE_WINDOW(w) &(w)
#else
#   define NATIVE_WINDOW(w) (w)
#endif

    /* Create ELG surface based on the native window */
    surface = eglCreateWindowSurface( display, config, NATIVE_WINDOW(nativeWindow), NULL );
    if( surface == EGL_NO_SURFACE ) throw std::runtime_error("Cannot create surface");

    /* Get surface context */
    result = eglMakeCurrent(display, surface, surface, context);
    if( result == EGL_FALSE ) throw std::runtime_error("Cannot connect context to surface");

    /* Save widget coordinates */
    x = sx;
    y = sy;
}

/* Load text file as a string */
std::string EGLWidget::loadFile(const char* file) {
    std::ifstream is(file);
    if( !is.is_open() ) throw std::runtime_error(std::string("Cannot open shader file: ") + file);

    std::string shader((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
    is.close();

    return shader;
}

/* Compile shader programe from the given file */
GLuint EGLWidget::compileShader(GLenum type, const char* file) {
    printf("Compiling shader file: %s\n", file);

    /* Create either vertex or pixel shader */
    GLuint shader = glCreateShader(type);
    if( !shader ) throw std::runtime_error(std::string("Cannot create shader for file: ") + file);

    /* Load shader code */
    std::string data = loadFile(file);
    const char* source[1] = { data.c_str() };

    /* Compile shader code */
    glShaderSource(shader, 1, source, NULL);
    glCompileShader(shader);

    /* Dont forget to check for errors */
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if( !status ) {
        char log[1024];
        GLsizei len;
        glGetShaderInfoLog(shader, sizeof(log), &len, log);
        glDeleteShader(shader);
        /* Throw exception with error description */
        throw std::runtime_error(std::string("Cannot compile shader from file: ") + file + ":\n" + log);
    }

    /* Return compiled shader descriptor */
    return shader;
}

/* Load vertex and pixel shaders */
void EGLWidget::loadShaders() {
    printf("Loading shaders\n");

    /* Compile shaders. Child class must provide us with path to shader files */ 
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragmentShader());
    GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertexShader());

    /* Create shader program */
    program = glCreateProgram();
    if( program == 0 ) throw std::runtime_error("Cannot create program");

    /* Link program with pixel and vertex shaders */
    glAttachShader(program, fragShader);
    glAttachShader(program, vertShader);
    glLinkProgram(program);

    /* Dont forget to check for errors */
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if( !status ) {
      char log[1024];
      GLsizei len;
      glGetProgramInfoLog(program, sizeof(log), &len, log);
      throw std::runtime_error(std::string("Cannot link program:\n") + log);
    }

    /* Tell EGL to use our shader program */
    glUseProgram(program);

    /* Get 'mvp' and 'frames' shader descriptors */
    u_mvp = glGetUniformLocation(program, "mvp");
    u_frames = glGetUniformLocation(program, "frames");

    /* Free resources */
    glDeleteShader(fragShader);
    glDeleteShader(vertShader);
}

/* Virtual function called before entering the main loop */
void EGLWidget::prepare() {
    /* Set viewport size */
    glViewport (0, 0, width, height);
}

/* Virtual function called to draw one frame */
void EGLWidget::draw() {
    /* Clear surface */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Pass frames counter to shader program */
    glUniform1f(u_frames, frames);
}

/* Virtual function returns path to vertex shader file */
const char* EGLWidget::vertexShader() {
    return "vertex.shader";
}
/* Virtual function returns path to pixel shader file */
const char* EGLWidget::fragmentShader() {
    return "fragment.shader";
}

/* Main loop */
void EGLWidget::run(int fps) {
    /* Load shaders */
    loadShaders();

    /* Prepare to enter the main loop */
    prepare();

    /* Given FPS calculate one frame duration */
    struct timeval tv1, tv2;
    const double frame_max = 1000000.0 / fps;

    while (1) {
        /* Frame start time */
        gettimeofday(&tv1, NULL);
        double t1 = tv1.tv_sec * 1000000 + tv1.tv_usec;

        /* Draw one frame */
        draw();
        frames += 1;

        /* Draw image on the screen, get frame end time */
        eglSwapBuffers(display, surface);
        gettimeofday(&tv2, NULL);

        /* If our frame took less time to draw - sleep the remaining time */
        double t2 = tv2.tv_sec * 1000000 + tv2.tv_usec;
        double delta = t2 - t1;

        if( delta < frame_max ) usleep(frame_max - delta);
    }
}
