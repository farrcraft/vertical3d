#version 450
#extension GL_GOOGLE_include_directive : require

/**
 * The fragment half of the lit pass: a key light and a fill, quantised into three flat bands,
 * with a cast shadow dropping a fragment one band.
 *
 * Everything here is linear light, and the target encodes it on store - ADR-0066. The albedo
 * was uploaded as sRGB, so it arrives here decoded.
 **/

#include "lit.glsl"

layout(location = 0) in vec3 worldNormal;
layout(location = 1) in vec2 fragmentUv;
layout(location = 2) in vec3 worldPosition;

layout(set = 1, binding = 0) uniform sampler2D albedo;
layout(set = 2, binding = 1) uniform sampler2D shadowMap;

layout(location = 0) out vec4 colour;

/**
 * How much of a 3x3 neighbourhood of the shadow map lies nearer the light than this fragment,
 * scaled by the shadow's strength. Zero is fully lit.
 **/
float occlusion(vec3 position, vec3 normal) {
    // moving the lookup off the surface along its normal is what removes the acne that depth
    // bias leaves on curved geometry
    const vec4 clip = scene.lightViewProjection * vec4(position + normal * scene.shadow.z, 1.0);
    const vec3 ndc = clip.xyz / clip.w;
    const vec2 at = ndc.xy * 0.5 + 0.5;

    // outside what the shadow map covers there is no depth to compare against, and it is lit
    // rather than shadowed so that the map's edge does not read as a wall
    if (ndc.z > 1.0 || any(lessThan(at, vec2(0.0))) || any(greaterThan(at, vec2(1.0)))) {
        return 0.0;
    }

    float occluded = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            const float nearest = texture(shadowMap, at + vec2(x, y) * scene.shadow.x).r;
            occluded += ndc.z > nearest ? 1.0 : 0.0;
        }
    }
    return (occluded / 9.0) * scene.shadow.y;
}

void main() {
    const vec3 n = normalize(worldNormal);
    const float key = max(dot(n, scene.light.xyz), 0.0);

    // the fill arrives from above and from opposite the key, which is where sky and bounce
    // reach the shadowed side from. A fill from straight up would give every upright face the
    // same value, so the faces of a figure lit from behind would stay one flat shape
    const vec3 fillDirection = normalize(vec3(-scene.light.x, 1.0, -scene.light.z));
    const float light = key + scene.light.w * (dot(n, fillDirection) * 0.5 + 0.5);

    const vec3 bands[3] = vec3[3](scene.bands.x * scene.shadowColour.rgb, scene.bands.y * scene.colour.rgb,
        scene.bands.z * scene.colour.rgb);
    const int band = light < scene.thresholds.x ? 0 : (light < scene.thresholds.y ? 1 : 2);

    // a shadow drops one band rather than darkening by a fraction, so cast shadows land on
    // the same palette as the shading. The neighbourhood only softens the edge between them
    const vec3 shade = mix(bands[band], bands[max(band - 1, 0)], occlusion(worldPosition, n));

    // an untextured model samples a white albedo, so the base colour alone tints it
    const vec4 base = texture(albedo, fragmentUv) * object.baseColour;
    colour = vec4(base.rgb * shade, base.a);
}
