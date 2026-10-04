/**
 * The blocks every lit shader shares, written once - ADR-0067. A shader includes this rather
 * than declaring its own, so a member added here is added everywhere at once.
 *
 * Set 0 is the camera every pipeline in the engine declares (ADR-0008), set 1 the albedo, and
 * set 2 the scene a lit pass binds once for itself (ADR-0064).
 **/

// set 0 - the camera the whole pass draws through
layout(std140, set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 viewport;
} camera;

// set 2 - the light, the cel bands and the shadow terms, laid out as realtime::SceneUniforms
layout(std140, set = 2, binding = 0) uniform Scene {
    vec4 light;                 // xyz towards the key light, normalised; w the fill
    vec4 thresholds;            // x shadow to mid, y mid to lit
    vec4 bands;                 // xyz the shadow, mid and lit multipliers
    mat4 lightViewProjection;   // what the shadow map was drawn through
    vec4 shadow;                // x a shadow map texel in uv, y strength, z normal bias
    vec4 colour;                // xyz the light's colour, over the mid and lit bands
    vec4 shadowColour;          // xyz the shadow band's colour
} scene;

// per object, laid out as renderer::Lit::Object. Declared whole in every stage so that one
// push range covers them all
layout(push_constant) uniform Object {
    mat4 model;
    vec4 baseColour;
    float outline;
    uint firstJoint;            // where the object's palette starts in skin.glsl's - ADR-0071
} object;
