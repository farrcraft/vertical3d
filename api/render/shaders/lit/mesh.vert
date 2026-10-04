#version 450
#extension GL_GOOGLE_include_directive : require

/**
 * The vertex half of the lit pass: a registered model's vertex, moved into the world by the
 * object's model matrix and drawn through the pass's camera. The world position goes on to
 * the fragment stage, which looks it up in the shadow map.
 **/

#include "lit.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 worldNormal;
layout(location = 1) out vec2 fragmentUv;
layout(location = 2) out vec3 worldPosition;

void main() {
    const vec4 world = object.model * vec4(position, 1.0);
    worldPosition = world.xyz;
    // the inverse transpose, so that a model scaled unevenly keeps its normals perpendicular
    worldNormal = mat3(transpose(inverse(object.model))) * normal;
    fragmentUv = uv;
    gl_Position = camera.viewProjection * world;
}
