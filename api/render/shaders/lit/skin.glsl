/**
 * What a skinned lit shader adds to lit.glsl: one storage buffer holding every joint palette
 * drawn this frame, and the matrix a vertex is moved by. Included after lit.glsl, whose Object
 * block gives where this object's palette starts.
 **/

// set 2 - every joint matrix drawn this frame, one palette after another
layout(std430, set = 2, binding = 2) readonly buffer Palette {
    mat4 joints[];
} palette;

layout(location = 3) in uvec4 joints;
layout(location = 4) in vec4 weights;

/**
 * The weighted sum of the vertex's joints' matrices, which takes it from where it was bound to
 * where the pose puts it, in the model's own space.
 **/
mat4 skinning() {
    return weights.x * palette.joints[object.firstJoint + joints.x] +
        weights.y * palette.joints[object.firstJoint + joints.y] +
        weights.z * palette.joints[object.firstJoint + joints.z] +
        weights.w * palette.joints[object.firstJoint + joints.w];
}
