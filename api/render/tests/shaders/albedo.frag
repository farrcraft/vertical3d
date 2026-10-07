#version 450

/**
 * Writes the texture bound at set 1, read at its centre, so a picture of it says which
 * material reached the draw. Set 0 is declared by the pipeline and read by nothing here.
 **/

layout(set = 1, binding = 0) uniform sampler2D albedo;

layout(location = 0) out vec4 colour;

void main() {
    colour = texture(albedo, vec2(0.5, 0.5));
}
