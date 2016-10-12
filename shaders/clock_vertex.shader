uniform mat4 mvp;
uniform float frames;

attribute vec4 vertex_xyz;
attribute vec2 vertex_st;
varying vec2 v_st;

void main() {
    gl_Position = mvp * vertex_xyz;
    v_st = vertex_st;
}
