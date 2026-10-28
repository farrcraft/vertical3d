#version 450
#extension GL_EXT_nonuniform_qualifier : require

/**
 * Writes the texture at the pushed slot of the array at set 1, read at its centre, so a picture
 * of it says which texture reached the draw. Set 0 is declared by the pipeline and read by
 * nothing here.
 **/

layout(set = 1, binding = 0) uniform sampler2D textures[];

layout(push_constant) uniform Push {
    uint slot;
} push;

layout(location = 0) out vec4 colour;

void main() {
    colour = texture(textures[push.slot], vec2(0.5, 0.5));
}
