#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

/**
 * The fragment stage of the lit pass: a key light and a fill, quantised into three flat bands,
 * with a cast shadow dropping a fragment one band.
 *
 * Everything here is linear light, and the sRGB target encodes it on store. The albedo was
 * uploaded as sRGB, so it arrives here decoded.
 **/

#include "lit.glsl"

layout(location = 0) in vec3 worldNormal;
layout(location = 1) in vec2 fragmentUv;
layout(location = 2) in vec3 worldPosition;
layout(location = 3) flat in uint instance;

// every texture the context holds. An object's albedo is the one at its slot, and the slot may
// differ between the fragments of one draw's instances
layout(set = 1, binding = 0) uniform sampler2D textures[];
// a layer for each cascade
layout(set = 2, binding = 1) uniform sampler2DArray shadowMap;

layout(location = 0) out vec4 colour;

/**
 * How much of a 3x3 neighbourhood of one cascade's layer lies nearer the light than this
 * fragment. Zero is fully lit.
 **/
float occluded(vec3 position, vec3 normal, int cascade) {
    // moving the lookup off the surface along its normal removes the acne that depth bias
    // leaves on curved geometry. A wider cascade has wider texels, so it moves further
    const float bias = scene.shadow.z * (scene.radii[cascade] / scene.radii[0]);
    const vec4 clip = scene.lightViewProjection[cascade] * vec4(position + normal * bias, 1.0);
    const vec3 ndc = clip.xyz / clip.w;
    const vec2 at = ndc.xy * 0.5 + 0.5;

    // outside the shadow map there is no depth to compare against. Such a fragment is lit
    // rather than shadowed, so the map's edge does not show as a wall
    if (ndc.z > 1.0 || any(lessThan(at, vec2(0.0))) || any(greaterThan(at, vec2(1.0)))) {
        return 0.0;
    }

    float hidden = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            const float nearest = texture(shadowMap, vec3(at + vec2(x, y) * scene.shadow.x, float(cascade))).r;
            hidden += ndc.z > nearest ? 1.0 : 0.0;
        }
    }
    return hidden / 9.0;
}

/**
 * How shadowed a fragment is, scaled by the shadow's strength. Zero is fully lit.
 *
 * The cascade is the first whose far end is beyond the fragment's distance from the camera's
 * near plane, and past the last there is no shadow. Over the last part of a cascade, the blend,
 * the fragment fades into the next one, so the change in detail between them does not show as
 * a line.
 **/
float occlusion(vec3 position, vec3 normal) {
    const float distance = dot(scene.plane.xyz, position) + scene.plane.w;
    const int count = int(scene.cascades.x);
    int cascade = 0;
    while (cascade < count && distance >= scene.far[cascade]) {
        ++cascade;
    }
    if (cascade >= count) {
        return 0.0;
    }

    float shadowed = occluded(position, normal, cascade);
    const float band = scene.far[cascade] * scene.cascades.y;
    const float into = distance - (scene.far[cascade] - band);
    if (cascade + 1 < count && band > 0.0 && into > 0.0) {
        shadowed = mix(shadowed, occluded(position, normal, cascade + 1), into / band);
    }
    return shadowed * scene.shadow.y;
}

void main() {
    const vec3 n = normalize(worldNormal);
    const float key = max(dot(n, scene.light.xyz), 0.0);

    // the fill arrives from above and from opposite the key, as sky and bounce light reach the
    // shadowed side. A fill from straight up would give every upright face the same value, and
    // a figure lit from behind would read as one flat shape
    const vec3 fillDirection = normalize(vec3(-scene.light.x, 1.0, -scene.light.z));
    const float light = key + scene.light.w * (dot(n, fillDirection) * 0.5 + 0.5);

    const vec3 bands[3] = vec3[3](scene.bands.x * scene.shadowColour.rgb, scene.bands.y * scene.colour.rgb,
        scene.bands.z * scene.colour.rgb);
    const int band = light < scene.thresholds.x ? 0 : (light < scene.thresholds.y ? 1 : 2);

    // a shadow drops one band rather than darkening by a fraction, so cast shadows land on
    // the same palette as the shading. The neighbourhood only softens the edge between them
    const vec3 shade = mix(bands[band], bands[max(band - 1, 0)], occlusion(worldPosition, n));

    // an untextured model samples a white albedo, so the base colour alone tints it
    const vec4 base = texture(textures[nonuniformEXT(objects.entries[instance].albedo)], fragmentUv) *
        objects.entries[instance].baseColour;
    colour = vec4(base.rgb * shade, base.a);
}
