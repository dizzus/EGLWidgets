precision mediump float;
uniform float frames;

void main() {
    float r = 1.5 * sin(0.01 * frames);
    float g = 1.4 * cos(0.02 * frames);
    float b = 1.3 * cos(0.03 * frames);
    gl_FragColor = vec4(r, g, b, 1);
}
