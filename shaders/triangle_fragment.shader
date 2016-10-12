precision mediump float;

uniform float frames;
varying vec4 v_color;

void main() {
    float r = v_color.r ;
    float g = v_color.g * cos(frames);
    float b = v_color.b * sin(frames);
    gl_FragColor = vec4(r, g, b, 1);
}
