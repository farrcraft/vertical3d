#version 450

/**
 * The vertex half of the editor's shaded pass: a mesh's triangles, lit by a light at the eye.
 *
 * The mesh's placement comes in a push constant, so a mesh is uploaded once and drawn wherever
 * its item is. The normal is carried into view space, where the eye looks along z.
 **/

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 viewNormal;

// set 0, the camera the whole pass draws through, updated once per frame
layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 viewport;
} camera;

// the item's placement, and the colour its surface is drawn in
layout(push_constant) uniform Object {
    mat4 model;
    vec4 colour;
} object;

void main() {
    // a normal is carried by the inverse transpose, so a scale along one axis still leaves it
    // square to the surface
    mat3 toWorld = transpose(inverse(mat3(object.model)));
    viewNormal = mat3(camera.view) * (toWorld * normal);
    gl_Position = camera.viewProjection * object.model * vec4(position, 1.0);
    // a pipeline that draws a triangle as its corners needs a point size, and one pixel is the
    // only size every device offers
    gl_PointSize = 1.0;
}
