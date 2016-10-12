precision mediump float;

uniform float frames;
uniform sampler2D u_texture;
varying vec2 v_st;

void main() {
    vec2 flipped = vec2(v_st.x, 1.0 - v_st.y);
    gl_FragColor = texture2D(u_texture, flipped);
}
