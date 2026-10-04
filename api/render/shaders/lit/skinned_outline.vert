#version 450
#extension GL_GOOGLE_include_directive : require

/**
 * outline.vert for a skinned model: the hull pushed out along the normal at the bind pose, and
 * then posed, so that it follows the surface it outlines.
 **/

#include "lit.glsl"
#include "skin.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

void main() {
    gl_Position = camera.viewProjection * object.model * skinning() * vec4(position + normal * object.outline, 1.0);
}
