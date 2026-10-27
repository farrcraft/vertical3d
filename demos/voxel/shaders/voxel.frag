#version 450

/**
 * The fragment half of the voxel terrain pipeline.
 *
 * A block face is flat and untextured, so everything that varies across it was worked out
 * per vertex and interpolated here.
 **/

layout(location = 0) in vec3 intensity;

layout(location = 0) out vec4 outColour;

void main() {
    outColour = vec4(intensity, 1.0);
}
