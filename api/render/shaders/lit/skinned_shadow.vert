#version 450
#extension GL_GOOGLE_include_directive : require

/**
 * shadow.vert for a skinned model, so that a posed character casts its pose rather than the
 * one it was bound in.
 **/

#include "lit.glsl"
#include "skin.glsl"

layout(location = 0) in vec3 position;

void main() {
    gl_Position = scene.lightViewProjection * object.model * skinning() * vec4(position, 1.0);
}
