/**
 * A colour glyph drawn from its layers' curves, as a program stored among the band values.
 *
 * A program starts at its value p. Value p is how many steps it has, and its steps start at
 * p + 4, STEP values each. A step's value 0 is its kind: a fill, or a group's beginning or end.
 *
 * - An end's values 1 and 2 are its composite and its opacity.
 * - A fill's value 3 is how many clips it has, values 4 to 6 the values their bands start at,
 *   and bit c of value 7 whether clip c is filled by the even-odd rule.
 * - Value 8 is the fill's kind: solid, linear, radial or sweep. Value 9 is how it extends past
 *   its stops. Value 10 is how many stops it has, and value 11 the value they start at, STOP
 *   values each: offset, red, green, blue, alpha, and 1 for the text's colour.
 * - Values 12 to 17 map em units into the gradient's space: x' = a x + c y + e, and
 *   y' = b x + d y + f.
 * - Values 18 to 23 are the gradient's start or centre, its end or second centre, and its
 *   radii or a sweep's angles.
 *
 * Each fill is drawn inside all its clips, over what is beneath it. A group gathers its steps
 * apart and is put on what is beneath it by its composite. Colours are blended as they are
 * encoded, premultiplied, and a gradient mixes its stops premultiplied.
 *
 * The file is included after curves.glsl.
 **/

// a vertex's glyph value with this bit names a program rather than a glyph
const uint COLOUR_PROGRAM = 0x80000000u;

const uint STEP = 24u;
const uint STOP = 6u;
const uint PROGRAM_HEADER = 4u;
const uint GROUPS = 4u;

const uint BEGIN_GROUP = 2u;
const uint END_GROUP = 3u;

const uint SOLID = 0u;
const uint LINEAR = 1u;
const uint RADIAL = 2u;
const uint SWEEP = 3u;

const uint REPEAT = 1u;
const uint REFLECT = 2u;

const float TAU = 6.28318530718;

// a stop's colour, premultiplied, in the text's colour when it asks for it
vec4 stopColour(uint bands, uint stop, vec4 foreground) {
    vec4 colour = vec4(bandValue(bands, stop + 1u), bandValue(bands, stop + 2u), bandValue(bands, stop + 3u), bandValue(bands, stop + 4u));
    if (bandValue(bands, stop + 5u) > 0.5) {
        colour = vec4(foreground.rgb, colour.a * foreground.a);
    }
    return vec4(colour.rgb * colour.a, colour.a);
}

// a place along a gradient brought inside its first and last stops, as it extends
float extended(float place, float low, float high, uint extend) {
    if (high <= low) {
        return place;
    }
    float span = high - low;
    float along = (place - low) / span;
    if (extend == REPEAT) {
        return low + (fract(along) * span);
    }
    if (extend == REFLECT) {
        float folded = mod(along, 2.0);
        return low + ((folded > 1.0 ? 2.0 - folded : folded) * span);
    }
    return clamp(place, low, high);
}

// a gradient's colour at a place along it, premultiplied
vec4 alongStops(uint bands, uint stops, uint count, uint extend, float place, vec4 foreground) {
    float low = bandValue(bands, stops);
    float high = bandValue(bands, stops + ((count - 1u) * STOP));
    float at = extended(place, low, high, extend);
    vec4 before = stopColour(bands, stops, foreground);
    if (at <= low) {
        return before;
    }
    float beforeOffset = low;
    for (uint stop = 1u; stop < count; stop++) {
        uint value = stops + (stop * STOP);
        float offset = bandValue(bands, value);
        vec4 after = stopColour(bands, value, foreground);
        if (at <= offset) {
            float gap = offset - beforeOffset;
            return mix(before, after, gap > 0.0 ? (at - beforeOffset) / gap : 1.0);
        }
        before = after;
        beforeOffset = offset;
    }
    return before;
}

// where a point in a gradient's space is along it: x the place, y 1 where it paints at all
vec2 gradientPlace(uint bands, uint step, uint kind, vec2 point) {
    vec2 from = vec2(bandValue(bands, step + 18u), bandValue(bands, step + 19u));
    vec2 to = vec2(bandValue(bands, step + 20u), bandValue(bands, step + 21u));
    float first = bandValue(bands, step + 22u);
    float second = bandValue(bands, step + 23u);
    if (kind == LINEAR) {
        vec2 run = to - from;
        float runSquared = dot(run, run);
        return runSquared > 0.0 ? vec2(dot(point - from, run) / runSquared, 1.0) : vec2(0.0);
    }
    if (kind == SWEEP) {
        float span = second - first;
        float angle = atan(point.y - from.y, point.x - from.x);
        angle = angle < 0.0 ? angle + TAU : angle;
        return span != 0.0 ? vec2((angle - first) / span, 1.0) : vec2(0.0);
    }
    // the largest t whose circle, centred from + t (to - from) with a radius that is not
    // negative, passes through the point: quadratic t^2 - 2 slope t + constant = 0
    vec2 centres = to - from;
    vec2 offset = point - from;
    float grows = second - first;
    float quadratic = dot(centres, centres) - (grows * grows);
    float slope = dot(offset, centres) + (first * grows);
    float constant = dot(offset, offset) - (first * first);
    if (abs(quadratic) < 1.0 / 65536.0) {
        float only = slope != 0.0 ? constant / (2.0 * slope) : -1.0;
        return (slope != 0.0 && first + (only * grows) >= 0.0) ? vec2(only, 1.0) : vec2(0.0);
    }
    float discriminant = (slope * slope) - (quadratic * constant);
    if (discriminant < 0.0) {
        return vec2(0.0);
    }
    float root = sqrt(discriminant);
    float high = max((slope + root) / quadratic, (slope - root) / quadratic);
    float low = min((slope + root) / quadratic, (slope - root) / quadratic);
    if (first + (high * grows) >= 0.0) {
        return vec2(high, 1.0);
    }
    return first + (low * grows) >= 0.0 ? vec2(low, 1.0) : vec2(0.0);
}

// a fill's colour at a point in em units, premultiplied
vec4 shadeFill(uint bands, uint step, vec2 em, vec4 foreground) {
    uint kind = uint(bandValue(bands, step + 8u));
    uint count = uint(bandValue(bands, step + 10u));
    uint stops = uint(bandValue(bands, step + 11u));
    if (count == 0u) {
        return vec4(0.0);
    }
    if (kind == SOLID) {
        return stopColour(bands, stops, foreground);
    }
    vec2 point = vec2((bandValue(bands, step + 12u) * em.x) + (bandValue(bands, step + 14u) * em.y) + bandValue(bands, step + 16u),
        (bandValue(bands, step + 13u) * em.x) + (bandValue(bands, step + 15u) * em.y) + bandValue(bands, step + 17u));
    vec2 place = gradientPlace(bands, step, kind, point);
    return place.y > 0.0 ? alongStops(bands, stops, count, uint(bandValue(bands, step + 9u)), place.x, foreground) : vec4(0.0);
}

// how much of a source and of a destination a Porter and Duff mode keeps, or below 0 for a mode
// that blends
vec2 porterDuff(uint mode, float source, float destination) {
    switch (mode) {
    case 0u: return vec2(0.0, 0.0);
    case 1u: return vec2(1.0, 0.0);
    case 2u: return vec2(0.0, 1.0);
    case 3u: return vec2(1.0, 1.0 - source);
    case 4u: return vec2(1.0 - destination, 1.0);
    case 5u: return vec2(destination, 0.0);
    case 6u: return vec2(0.0, source);
    case 7u: return vec2(1.0 - destination, 0.0);
    case 8u: return vec2(0.0, 1.0 - source);
    case 9u: return vec2(destination, 1.0 - source);
    case 10u: return vec2(1.0 - destination, source);
    case 11u: return vec2(1.0 - destination, 1.0 - source);
    case 12u: return vec2(1.0, 1.0);
    default: return vec2(-1.0);
    }
}

float hardLightChannel(float backdrop, float source) {
    return source <= 0.5 ? backdrop * 2.0 * source : backdrop + ((2.0 * source) - 1.0) - (backdrop * ((2.0 * source) - 1.0));
}

float softLightChannel(float backdrop, float source) {
    if (source <= 0.5) {
        return backdrop - ((1.0 - (2.0 * source)) * backdrop * (1.0 - backdrop));
    }
    float lifted = backdrop <= 0.25 ? ((((16.0 * backdrop) - 12.0) * backdrop) + 4.0) * backdrop : sqrt(backdrop);
    return backdrop + (((2.0 * source) - 1.0) * (lifted - backdrop));
}

float separableChannel(uint mode, float backdrop, float source) {
    switch (mode) {
    case 13u: return backdrop + source - (backdrop * source);
    case 14u: return hardLightChannel(source, backdrop);
    case 15u: return min(backdrop, source);
    case 16u: return max(backdrop, source);
    case 17u: return backdrop <= 0.0 ? 0.0 : (source >= 1.0 ? 1.0 : min(1.0, backdrop / (1.0 - source)));
    case 18u: return backdrop >= 1.0 ? 1.0 : (source <= 0.0 ? 0.0 : 1.0 - min(1.0, (1.0 - backdrop) / source));
    case 19u: return hardLightChannel(backdrop, source);
    case 20u: return softLightChannel(backdrop, source);
    case 21u: return abs(backdrop - source);
    case 22u: return backdrop + source - (2.0 * backdrop * source);
    default: return backdrop * source;
    }
}

float luminosityOf(vec3 colour) {
    return dot(colour, vec3(0.3, 0.59, 0.11));
}

vec3 withLuminosity(vec3 colour, float lum) {
    vec3 moved = colour + (lum - luminosityOf(colour));
    float l = luminosityOf(moved);
    float low = min(min(moved.r, moved.g), moved.b);
    float high = max(max(moved.r, moved.g), moved.b);
    if (low < 0.0 && l != low) {
        moved = l + (((moved - l) * l) / (l - low));
    }
    if (high > 1.0 && high != l) {
        moved = l + (((moved - l) * (1.0 - l)) / (high - l));
    }
    return moved;
}

float saturationOf(vec3 colour) {
    return max(max(colour.r, colour.g), colour.b) - min(min(colour.r, colour.g), colour.b);
}

vec3 withSaturation(vec3 colour, float sat) {
    float low = min(min(colour.r, colour.g), colour.b);
    float high = max(max(colour.r, colour.g), colour.b);
    return high > low ? ((colour - low) * sat) / (high - low) : vec3(0.0);
}

vec3 blendColour(uint mode, vec3 backdrop, vec3 source) {
    switch (mode) {
    case 24u: return withLuminosity(withSaturation(source, saturationOf(backdrop)), luminosityOf(backdrop));
    case 25u: return withLuminosity(withSaturation(backdrop, saturationOf(source)), luminosityOf(backdrop));
    case 26u: return withLuminosity(source, luminosityOf(backdrop));
    case 27u: return withLuminosity(backdrop, luminosityOf(source));
    default:
        return vec3(separableChannel(mode, backdrop.r, source.r), separableChannel(mode, backdrop.g, source.g),
            separableChannel(mode, backdrop.b, source.b));
    }
}

// one premultiplied colour put on another by a composite mode
vec4 compositeColour(vec4 source, vec4 destination, uint mode) {
    vec2 keeps = porterDuff(mode, source.a, destination.a);
    if (keeps.x >= 0.0) {
        return min((source * keeps.x) + (destination * keeps.y), vec4(1.0));
    }
    vec3 backdrop = destination.a > 0.0 ? destination.rgb / destination.a : vec3(0.0);
    vec3 painted = source.a > 0.0 ? source.rgb / source.a : vec3(0.0);
    vec3 mixed = ((1.0 - destination.a) * source.rgb) + ((1.0 - source.a) * destination.rgb) + (source.a * destination.a * blendColour(mode, backdrop, painted));
    return vec4(mixed, source.a + destination.a - (source.a * destination.a));
}

/**
 * @param em the sample's place in the glyph, in em units with y upwards
 * @param program the value the program starts at, without the bit that marks it
 * @param foreground the text's colour, as it is displayed, which a layer may take
 * @param pixelsPerEm the em square's size in pixels along each axis, found where every sample
 *        of a quad reaches it
 * @return the glyph's colour at the sample, premultiplied, as it is displayed
 **/
vec4 colourGlyph(uint curves, uint bands, vec2 em, uint program, vec4 foreground, vec2 pixelsPerEm) {
    vec4 groups[GROUPS];
    groups[0] = vec4(0.0);
    uint top = 0u;
    // a group past the deepest is drawn into the one around it, and its end is passed over
    uint deeper = 0u;
    uint steps = uint(bandValue(bands, program));
    for (uint index = 0u; index < steps; index++) {
        uint step = program + PROGRAM_HEADER + (index * STEP);
        uint kind = uint(bandValue(bands, step));
        if (kind == BEGIN_GROUP) {
            if (top + 1u < GROUPS) {
                top++;
                groups[top] = vec4(0.0);
            } else {
                deeper++;
            }
            continue;
        }
        if (kind == END_GROUP) {
            if (deeper > 0u) {
                deeper--;
            } else if (top > 0u) {
                vec4 gathered = groups[top] * bandValue(bands, step + 2u);
                top--;
                groups[top] = compositeColour(gathered, groups[top], uint(bandValue(bands, step + 1u)));
            }
            continue;
        }
        float inside = 1.0;
        uint clips = min(uint(bandValue(bands, step + 3u)), 3u);
        uint evenOdd = uint(bandValue(bands, step + 7u));
        for (uint clip = 0u; clip < clips && inside > 0.0; clip++) {
            inside *= curveCoverageAt(curves, bands, em, uint(bandValue(bands, step + 4u + clip)), pixelsPerEm, ((evenOdd >> clip) & 1u) != 0u);
        }
        if (inside > 0.0) {
            vec4 painted = shadeFill(bands, step, em, foreground) * inside;
            groups[top] = painted + (groups[top] * (1.0 - painted.a));
        }
    }
    return groups[0];
}
