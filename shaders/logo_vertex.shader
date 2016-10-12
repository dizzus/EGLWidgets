uniform mat4 mvp;
uniform float frames;
attribute vec4 pos;

void main() {
    gl_Position = mvp * pos;
}

