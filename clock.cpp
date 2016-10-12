#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdexcept>
#include <string>

#include <algorithm>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "clock.h"

using namespace glm;

/* Vertex and pixel shaders */
const char* Clock::vertexShader() {
    return "shaders/clock_vertex.shader";
}

const char* Clock::fragmentShader() {
    return "shaders/clock_fragment.shader";
}

/* Plane made of two triangles */
/*
  B +----------+ A          x: +1           t: 1
    |         /|            |                |
    |       /  |            |                |
    |     /    |  y: -1-----+----- y: +1     |
    |   /      |            |                |
    | /        |            |                +----------- s: 1
  C +----------+ D         x: -1             0
*/

/* Vertex coordinates */
static const GLfloat verts[] = {
    +1, +1, 0, 1, 1,            /* A (x: +1, y: +1, z: 0, t: 1, s: 1) */
    -1, +1, 0, 0, 1,            /* B (x: -1, y: +1, z: 0, t: 0, s: 1) */
    -1, -1, 0, 0, 0,            /* C (x: -1, y: -1, z: 0, t: 0, s: 0) */
    +1, -1, 0, 1, 0,            /* D (x: +1, y: -1, z: 0, t: 1, s: 0) */
};

/* Vertex indexes */
static const GLushort indexes[] = {
    0, 1, 2,                    /* A -> B -> C */
    0, 2, 3,                    /* A -> C -> D */
};

/* Initialization before the main loop */
void Clock::prepare() {
    /* Calling parent */
    EGLWidget::prepare();

    /* We want transparency */
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Create buffer and load vertex data */
    GLuint vert_buf;
    glGenBuffers(1, &vert_buf);
    glBindBuffer(GL_ARRAY_BUFFER, vert_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    /* Create 'vertex_xyz' shader parameter which represent vertex coodrinates */
    v_xyz = glGetAttribLocation(program, "vertex_xyz");
    glEnableVertexAttribArray(v_xyz);
    /* Vertex structure: 3 float-s per coordinate, total 5 floats, coordinate data starts at index 0 */
    glVertexAttribPointer(v_xyz, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);

    /* Prepare shader parameter 'vertex"st' which represent texture coordinates */
    v_st  = glGetAttribLocation(program, "vertex_st");
    glEnableVertexAttribArray(v_st);
    /* Texture structure: 2 floats per texture coordinate, total 5 floats, texture data starts at index 3 */
    glVertexAttribPointer(v_st, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    /* Load our triangle into the buffer */
    GLuint index_buf;
    glGenBuffers(1, &index_buf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes), indexes, GL_STATIC_DRAW);

    /* Zero-ing the texture and enable it */
    memset( clock_texture, 0, sizeof(clock_texture) );
    glEnable (GL_TEXTURE_2D);

    /* Create a texture buffer */
    glGenTextures(1, &texture_id);
    assert(texture_id != 0);

    /* Bind texture buffer */
    glBindTexture(GL_TEXTURE_2D, texture_id);
    /* Describe our texture */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    /* Loading texture data: 256x256 pixels, RGBA format */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, clock_texture);

    /* Get 'u_texture' shader parameter descriptor */
    u_texture = glGetUniformLocation(program, "u_texture");
    printf("Texture id: %d, u_texture: %d\n", texture_id, u_texture);
}

/* Draw text given font size and pen coordinates */
void Clock::printText(const char* text, int pen_x, int pen_y, int size) {
    /* Make sure the font supports our size */
    assert( FT_Set_Pixel_Sizes(face, 0, size) == 0 );

    /* All work is done using so called "glyph slot" */
    FT_GlyphSlot g = face->glyph;

    /* Draw text one symbol at a time */
    for( int i = 0; text[i] != 0; ++i ) {
        /* Load glyph data */
        assert( FT_Load_Char(face, text[i], FT_LOAD_RENDER) == 0 );

        /* Glyph size */
        int gw = g->bitmap.width;
        int gh = g->bitmap.rows;

        /* Texture coordinate */
        int texture_x = pen_x + g->bitmap_left;
        int texture_y = pen_y - g->bitmap_top;

        /* Copy glyph bitmap */
        for( int row = 0; row < gh; ++row ) {
            for( int col = 0; col < gw; ++col ) {
                /* Get on pixel from the glyph. Pixel is encoded as 0..255 grey scale*/
                unsigned char color = g->bitmap.buffer[ col + row * gw ];

                /* Where do we have to copy our pixel */
                pixel_t& pixel = clock_texture[ texture_x + col + (texture_y + row) * 256 ];
                /* Pixel color: #FF6600 */
                pixel.r = 255;
                pixel.g = 102;
                pixel.b = 0;
                /* Set alpha value equal to glyph pixel value */
                pixel.a = color;
            }
        }

        /* Move pen position using glyph's special 'advance' value, divided by 64 */
        pen_x += g->advance.x >> 6;
    }
}


/* Draw one frame */
void Clock::draw() {
    /* Call parent method*/
    EGLWidget::draw();

    /* Redraw clock texture only when time actually changed */
    time_t now = time(NULL);

    if( now != last_time ) {
        char text[64];
        struct tm * timeinfo = localtime(&now);
        strftime(text, sizeof(text), "%H:%M:%S", timeinfo);

        /* Hour:min:sec*/
        memset(clock_texture, 0, sizeof(clock_texture));
        printText(text, 50, 60, font_size);

        /* Date month */
        strftime(text, sizeof(text), "%d %b", timeinfo);
        printText(text, 140, 90, 30);

        /* Update clock texture with the new data */
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, clock_texture);
    }

    last_time = now;

    /* Calculate rotation and scaling matices */
    mat4 model;
    mat4 rotated = rotate(model, angle, vec3(0, 0, 1.0));
    mat4 scaled = scale(rotated, vec3(1, 1, 1));
    mat4 mvp = rotated * scaled;

    /* Pass updated MVP matrix to the shader */
    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

    /* Draw clock plane/texture */
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

/* Clock widget */
Clock::Clock(const char* font, int size): EGLWidget(0, 0, 400, 400) {
    /* Initialize FreeType library */
    assert( FT_Init_FreeType(&library) == 0 );
    assert( FT_New_Face(library, font, 0, &face) == 0 );

    font_size = size;

    /* Clock rotation. To rotate the clock face to 90 degrees use M_PI / 4.0 */
    angle = M_PI / 4.0;

    /* Initialize shader descriptors */
    v_xyz = 0;
    v_st = 0;
    texture_id = 0;
    u_texture = 0;

    /* Last update time */
    last_time = 0;
}

/* Free up resources */
Clock::~Clock() {
    FT_Done_FreeType(library);
}

int main(int argc, char** argv) {
#ifdef IS_RPI
    bcm_host_init();
#endif
    try {
        /* Command line parameters: font path and size. */
        Clock clock(argc > 1 ? argv[1] : "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf", 
                    argc > 2 ? atoi(argv[2]) : 50);
        clock.run(2);
    } catch (const std::exception& ex) {
        fprintf(stderr, "Error: %s", ex.what());
    }
}
