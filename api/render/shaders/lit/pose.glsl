/**
 * The only difference between a lit shader for a skinned model and the same shader for a rigid
 * one: the matrix that moves a vertex from where it is stored to where the model's pose puts it,
 * in the model's own space. A shader compiled with SKINNED takes it from the joints, and one
 * compiled without returns the identity. Included after lit.glsl.
 **/

#ifdef SKINNED
#include "skin.glsl"

mat4 pose() {
    return skinning();
}
#else
mat4 pose() {
    return mat4(1.0);
}
#endif
