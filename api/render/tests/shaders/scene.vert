#version 450

/**
 * One triangle that covers the whole of what the pass draws into, built from the vertex
 * index so that the draw needs no buffer. Every pixel is wholly inside it, so nothing about
 * the picture it makes depends on how an implementation rasterizes an edge - ADR-0054.
 **/

void main() {
    const vec2 corner = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
}
