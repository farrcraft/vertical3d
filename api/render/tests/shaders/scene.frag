#version 450

/**
 * Writes the colour bound at set 2 and nothing else, so a picture of it says whether the
 * pass's scene set reached the draw - ADR-0064. Sets 0 and 1 are declared by the pipeline
 * and read by nothing here.
 **/

layout(set = 2, binding = 0) uniform Scene {
    vec4 colour;
} scene;

layout(location = 0) out vec4 outColour;

void main() {
    outColour = scene.colour;
}
