#include <string.h>
#include <stdio.h>
#include "triangle.h"

/* Z-axis rotation matrix */
static void
make_z_rot_matrix(GLfloat angle, GLfloat *m) {
    float c = cos(angle * M_PI / 180.0);
    float s = sin(angle * M_PI / 180.0);

    memset(m, 0, 16 * sizeof(GLfloat));
    m[0] = m[5] = m[10] = m[15] = 1.0;

    m[0] = c;
    m[1] = s;
    m[4] = -s;
    m[5] = c;
}

/* Scale matrix */
static void
make_scale_matrix(GLfloat xs, GLfloat ys, GLfloat zs, GLfloat *m) {
    memset(m, 0, 16 * sizeof(GLfloat));
    m[0] = xs;
    m[5] = ys;
    m[10] = zs;
    m[15] = 1.0;
}

/* Multiply two matrices */
static void
mul_matrix(GLfloat *prod, const GLfloat *a, const GLfloat *b) {
#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col)  p[(col<<2)+row]
    GLfloat p[16];
    GLint i;
    for( i = 0; i < 4; ++i ) {
        const GLfloat ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
        P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
        P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
        P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
        P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
    }
    memcpy(prod, p, sizeof(p));
#undef A
#undef B
#undef PROD
}

/* Vertex shader file */
const char* Triangle::vertexShader() {
    return "shaders/triangle_vertex.shader";
}

/* Pixel shader file */
const char* Triangle::fragmentShader() {
    return "shaders/triangle_fragment.shader";
}

/* Initialization before the main loop */
void Triangle::prepare() {
   /* Triangle 2D-coordinates */
   static const GLfloat verts[][2] = {
        { -1, -1 },
        {  1, -1 },
        {  0,  1 }
    };
    /* Vertex RGB-colors */
    static const GLfloat colors[][3] = {
        { 1, 0, 0 },
        { 0, 1, 0 },
        { 0, 0, 1 }
    };

    /* Call parent */
    EGLWidget::prepare();

    /* Pass 'pos' and 'color' parameters into our shader program */
    glBindAttribLocation(program, attr_pos, "pos");
    glBindAttribLocation(program, attr_color, "color");
    /* Link shader program */
    glLinkProgram(program);

    /* Describe our vertex and color data */
    glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glVertexAttribPointer(attr_color, 3, GL_FLOAT, GL_FALSE, 0, colors);

    /* Enable attributes */
    glEnableVertexAttribArray(attr_pos);
    glEnableVertexAttribArray(attr_color);
}

/* Draw one frame */
void Triangle::draw() {
    /* MVP, rotation and scaling matrices */
    GLfloat mvp[16], rot[16], scale[16];

    /* Call parent */
    EGLWidget::draw();

    /* Calculate MVP matrix */
    make_z_rot_matrix(angle, rot);
    make_scale_matrix(0.6, 0.6, 0.6, scale);
    mul_matrix(mvp, rot, scale);

    /* Pass it to our shader */
    glUniformMatrix4fv(u_mvp, 1, GL_FALSE, mvp);
    /* Draw our trianlge */
    glDrawArrays(GL_TRIANGLES, 0, 3);

    /* Rotate */
    angle += 5.0;
}

/* Initialize widget */
Triangle::Triangle(): EGLWidget(100, 100, 400, 400) {
    angle = 0.0;
    /* Shader descriptor may be assigned by hand. Do it just for fun */
    attr_pos = 0;
    attr_color = 1;
}

int main(void) {
#ifdef IS_RPI
    bcm_host_init();
#endif
    try {
        Triangle triangle;
        triangle.run(15);
    } catch (const std::exception& ex) {
        fprintf(stderr, "Error: %s", ex.what());
    }
}
