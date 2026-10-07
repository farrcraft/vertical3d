#version 450

/**
 * One triangle that covers the whole target a pass draws into, from three vertices and no
 * vertex buffer. Its uv runs from 0 at the top left of the target to 1 at the bottom right. A
 * fragment stage reading set 1 at uv therefore reads the texel under the pixel, because
 * Vulkan's clip space has y down, and so does an image.
 **/

layout(location = 0) out vec2 uv;

void main() {
    // (0, 0), (2, 0) and (0, 2), whose triangle holds the unit square
    uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
