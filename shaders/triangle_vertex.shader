uniform mat4 mvp;
uniform float frames;

attribute vec4 pos;
attribute vec4 color;
varying vec4 v_color;

void main() {
    gl_Position = mvp * pos;
    gl_Position.x = gl_Position.x + sin(frames) * 0.1;
    v_color = color;
}

