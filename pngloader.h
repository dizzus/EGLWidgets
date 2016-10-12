#ifndef __PNGLOADER_H__
#define __PNGLOADER_H__

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

/* Create EGL texture out of PNG-file */
GLuint load_png_as_texture(const char* path);

/* Create EGL texture out of PNG-file, get image size */
GLuint load_png_as_texture(const char* path, size_t* width, size_t* height);

/* Load PNG-file, get image size */
char* load_png_image(const char* path, size_t* width, size_t* height);

#endif