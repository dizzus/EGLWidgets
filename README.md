# EGL Widgets

EGL Widget is a very simple C++ class to help you get started programming [**OpenGL ES**](https://en.wikipedia.org/wiki/OpenGL_ES).

It's a base class that implements all tedious initialization code for OpenGL ES system
and gives your subclass a simple interface to implement an on-screen widget, like: clock,
logo and basically any kind of OSD widget.

* Basic widget requires only one C++ file added to your dependencies.
* It should work both under X Window and Raspberry Pi BCM host.
* It comes with several example widgets.

Currently Raspberry Pi (2) is the only EGL platform on which the code has been tested.
Besides that you should be able to compile all your widgets on your desktop OS running X Window 
which is quite useful for debugging.
Porting to other platform should be pretty straightforward.


## Installation

Include _widget.cpp_ file into your project or Makefile.
If your widget uses .PNG-files as textures add _pngloader.cpp_ to your dependencies.
If your widget uses [**.OBJ-files**](https://en.wikipedia.org/wiki/Wavefront_.obj_file) as 3D meshes add _mesh.cpp_ to your dependencies.

You may choose to implement all matrix manipulation code by yourself but it's much easier
to use [**glm**](http://glm.g-truc.net/0.9.8/index.html) library. In such case you should have it installed on your system.

In case your widget needs to work with PNG-files your system should have [**libpng**](http://www.libpng.org/pub/png/libpng.html) installed.

## Basic Usage

Create a subclass derived from EGLWidget.
For example:

```c++
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

```

As a bare minimum - implement all OpenGL drawing code in **draw()** method. You should also implement both vertex and pixel shaders which in most cases can be simply copy-pasted from example code:
```c++
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
```

Basic vertex shader program. Override method **vertexShader()** in your subclass
to return path to this shader program file:

```c
uniform mat4 mvp;
uniform float frames;

attribute vec4 vertex_xyz;
attribute vec2 vertex_st;
varying vec2 v_st;

void main() {
    gl_Position = mvp * vertex_xyz;
    v_st = vertex_st;
}
```

Basic vertex shader program. Override method **fragmentShader()** in your subclass
to return path to this shader program file:

```c
precision mediump float;

uniform float frames;
uniform sampler2D u_texture;
varying vec2 v_st;

void main() {
    vec2 flipped = vec2(v_st.x, 1.0 - v_st.y);
    gl_FragColor = texture2D(u_texture, flipped);
}
```

Your widget most certainly will require some initialization - implement it in **prepare()** 
method. For example:

```c++
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
    /* Vertex structure: 3 float-s per coordinate, total 5 floats, coordinate data starts 
    at index 0 */
    glVertexAttribPointer(v_xyz, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);

    /* Prepare shader parameter 'vertex"st' which represent texture coordinates */
    v_st  = glGetAttribLocation(program, "vertex_st");
    glEnableVertexAttribArray(v_st);
    /* Texture structure: 2 floats per texture coordinate, total 5 floats, texture data 
    starts at index 3 */
    glVertexAttribPointer(v_st, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 
    (void*)(3 * sizeof(GLfloat)));

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
```

The base class automatically provides two shader parameters to your shader programms:
+ **frames** - frame counter which gets incremented on every frame draw 
+ **mvp** - Model-View-Projection matrix which your subclass should calculate in the **draw()** call 
(see example code above). 

## Examples

There're several example included with this library:

* clock.cpp - a widget that shows current system time.
* logo.cpp - a widget that shows a rotating 3d-logo.
* texture.cpp - a widget that shows a rotating 2d-logo.
* triangle.cpp - a widget that shows a rotating triangle. 

NB: _clock_ widget requires [**FreeType**](https://www.freetype.org) library installed. This widget may also serve you as a basic example on how to work with glyphs in FreeType library.

_triangle_ widget shows you how you can implement your own matrix manipulation code.

You can pass different parameter to widget on the command line. Please refer to the source code to
find out more.


## Contact

Dmitry Bakhvalov

- https://github.com/dizzus
- dizzus@gmail.com

## License

EGLWidgets is available under the MIT license. See the LICENSE file for more info.

