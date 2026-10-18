/**
 * The sRGB transfer functions, per channel. A shader includes this rather than writing its own,
 * so that every stage that converts agrees with the others to the last bit.
 **/

// display to linear: what an _SRGB target does when it is sampled
vec3 linear(vec3 shown) {
    vec3 low = shown / 12.92;
    vec3 high = pow((shown + 0.055) / 1.055, vec3(2.4));
    return mix(high, low, lessThanEqual(shown, vec3(0.04045)));
}

// linear to display: what an _SRGB target does when it is stored to
vec3 display(vec3 light) {
    vec3 low = light * 12.92;
    vec3 high = 1.055 * pow(light, vec3(1.0 / 2.4)) - 0.055;
    return mix(high, low, lessThanEqual(light, vec3(0.0031308)));
}
