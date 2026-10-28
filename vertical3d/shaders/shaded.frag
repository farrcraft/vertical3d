#version 450

/**
 * The fragment half of the editor's shaded pass.
 *
 * The light is at the eye, so a surface facing the view is brightest, and one seen edge on is
 * darkest. Either side of a surface is lit alike, so a back face a view shows is not black.
 **/

layout(location = 0) in vec3 viewNormal;

layout(location = 0) out vec4 colour;

layout(push_constant) uniform Object {
    mat4 model;
    vec4 colour;
} object;

// how much of the colour a surface seen edge on keeps
const float ambient = 0.3;

void main() {
    float facing = abs(normalize(viewNormal).z);
    colour = vec4(object.colour.rgb * (ambient + ((1.0 - ambient) * facing)), 1.0);
}
