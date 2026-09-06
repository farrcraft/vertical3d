#version 450

/**
 * The vertex half of the voxel terrain pipeline - ambient, diffuse and specular shading of
 * an axis aligned block face.
 *
 * Lighting is done here rather than per fragment because a face is flat: its normal is one
 * of six constants and its material is constant across the quad, so the four corners carry
 * everything the interpolator needs.
 *
 * Positions arrive in chunk local blocks and the chunk's origin comes in a push constant,
 * which is what lets a chunk be culled or moved without rebuilding its mesh.
 **/

layout(location = 0) in vec3 position;
// x is the face bit the vertex belongs to, y is the block type it was cut from
layout(location = 1) in vec2 info;

layout(location = 0) out vec3 intensity;

// set 0, the per frame frequency of ADR-0008 - the camera the whole pass draws through
layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 viewport;
} camera;

struct MaterialInfo {
    vec4 ambient;   // reflectivity in rgb
    vec4 diffuse;
    vec4 specular;  // reflectivity in rgb, the shininess exponent in w
};

// set 1, the per material frequency - the one light and the block palette, written once
layout(set = 1, binding = 0) uniform Scene {
    vec4 lightPosition;  // in world space
    vec4 ambient;        // light intensities
    vec4 diffuse;
    vec4 specular;
    MaterialInfo materials[16];
} scene;

layout(push_constant) uniform Push {
    vec4 origin;  // where the chunk sits in the world, in blocks
} push;

vec3 faceNormal(int face) {
    switch (face) {
        case 2:  return vec3(0.0, 0.0, 1.0);   // front
        case 4:  return vec3(-1.0, 0.0, 0.0);  // left
        case 8:  return vec3(1.0, 0.0, 0.0);   // right
        case 16: return vec3(0.0, 0.0, -1.0);  // back
        case 32: return vec3(0.0, 1.0, 0.0);   // top
        case 64: return vec3(0.0, -1.0, 0.0);  // bottom
        default: return vec3(0.0, 1.0, 0.0);
    }
}

void main() {
    vec3 world = position + push.origin.xyz;
    vec3 normal = faceNormal(int(info.x));
    // the palette is indexed from the block type, whose zero is air and is never meshed
    MaterialInfo material = scene.materials[clamp(int(info.y) - 1, 0, 15)];

    // the eye in world space. A view matrix is a rotation and a translation, so undoing it
    // is a transpose and a rotated, negated translation rather than a full inverse
    vec3 eye = -(transpose(mat3(camera.view)) * camera.view[3].xyz);

    vec3 toLight = normalize(scene.lightPosition.xyz - world);
    vec3 toEye = normalize(eye - world);
    vec3 reflected = reflect(-toLight, normal);

    float lambert = max(dot(toLight, normal), 0.0);
    vec3 shade = scene.ambient.rgb * material.ambient.rgb +
        scene.diffuse.rgb * material.diffuse.rgb * lambert;
    if (lambert > 0.0) {
        shade += scene.specular.rgb * material.specular.rgb * pow(max(dot(reflected, toEye), 0.0), material.specular.w);
    }

    intensity = shade;
    gl_Position = camera.viewProjection * vec4(world, 1.0);
}
