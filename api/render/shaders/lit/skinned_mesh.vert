#version 450
#extension GL_GOOGLE_include_directive : require

/**
 * mesh.vert for a skinned model: the vertex moved by its joints before the model matrix, and
 * its normal turned with it.
 **/

#include "lit.glsl"
#include "skin.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 worldNormal;
layout(location = 1) out vec2 fragmentUv;
layout(location = 2) out vec3 worldPosition;

void main() {
    const mat4 posed = object.model * skinning();
    const vec4 world = posed * vec4(position, 1.0);
    worldPosition = world.xyz;
    worldNormal = mat3(transpose(inverse(posed))) * normal;
    fragmentUv = uv;
    gl_Position = camera.viewProjection * world;
}
