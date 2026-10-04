#version 450
#extension GL_GOOGLE_include_directive : require

/**
 * The outline: the model pushed out along its normals in its own space, before the model
 * matrix, and drawn with its front faces culled. What survives is the back of a slightly larger
 * hull, visible only around the silhouette.
 **/

#include "lit.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

void main() {
    gl_Position = camera.viewProjection * object.model * vec4(position + normal * object.outline, 1.0);
}
