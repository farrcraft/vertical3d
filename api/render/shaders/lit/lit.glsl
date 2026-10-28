/**
 * The blocks every lit shader shares. A shader includes this rather than declaring its own, so
 * a member added here is added everywhere at once. A replacement shader handed to renderer::Lit
 * must declare the same sets, and can include this file to do so.
 *
 * Set 0 is the camera every pipeline in the engine declares, set 1 the albedo, and set 2 the
 * scene a lit pass binds once for itself.
 **/

// set 0 - the camera the whole pass draws through
layout(std140, set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 viewport;
} camera;

// the most cascades a shadow map has, as realtime::lighting::maximumCascades
#define MAXIMUM_CASCADES 4

// set 2 - the light, the cel bands and the shadow terms, laid out as
// realtime::lighting::SceneUniforms
layout(std140, set = 2, binding = 0) uniform Scene {
    vec4 light;                 // xyz towards the key light, normalised; w the fill
    vec4 thresholds;            // x shadow to mid, y mid to lit
    vec4 bands;                 // xyz the shadow, mid and lit multipliers
    mat4 lightViewProjection[MAXIMUM_CASCADES];  // what each cascade's layer was drawn through
    vec4 shadow;                // x a shadow map texel in uv, y strength, z normal bias
    vec4 colour;                // xyz the light's colour, over the mid and lit bands
    vec4 shadowColour;          // xyz the shadow band's colour
    vec4 far;                   // where each cascade ends, from the camera's near plane
    vec4 radii;                 // each cascade's radius, which scales its normal bias
    vec4 plane;                 // dot(xyz, p) + w is p's distance from the near plane
    vec4 cascades;              // x how many cascades, y the part of each that blends into the next
} scene;

// one object's data, laid out as renderer::Lit::Object
struct Object {
    mat4 model;
    vec4 baseColour;
    float outline;
    uint firstJoint;            // where the object's palette starts in skin.glsl's joint buffer
    uint albedo;                // the albedo's slot in the texture array at set 1
};

// set 2 - every object drawn this frame. A draw's first instance is where its objects start, so
// an instance's own object is the entry at its instance index
layout(std430, set = 2, binding = 3) readonly buffer Objects {
    Object entries[];
} objects;

// the object a vertex stage is drawing. A fragment stage has no instance index, so it is handed
// the index by its vertex stage and reads objects.entries itself
#define object objects.entries[gl_InstanceIndex]
