/**
 * A glyph's coverage found from its quadratic curves, as the Slug algorithm finds it.
 *
 * The curves and the bands are float textures read with texelFetch, four floats a texel, each
 * row as wide as its texture. Curve c is texels 2c and 2c + 1: p1 and p2, then p3. Band value k
 * is component k mod 4 of texel k / 4.
 *
 * A glyph's bands start at its value g. Values g to g + 3 are the band transform: x and y
 * scale, then x and y offset. Values g + 4 and g + 5 are how many vertical and horizontal bands
 * it has. From g + 8, each horizontal band has its curve count and the value its list starts
 * at, and then each vertical band has the same.
 *
 * A list holds curve indices, sorted from the curve reaching furthest along the band's ray.
 *
 * The file is included where the texture array is declared as textures[].
 **/

vec4 curveTexel(uint slot, uint index) {
    uint width = uint(textureSize(textures[slot], 0).x);
    return texelFetch(textures[slot], ivec2(int(index % width), int(index / width)), 0);
}

float bandValue(uint slot, uint index) {
    return curveTexel(slot, index >> 2u)[index & 3u];
}

// which roots of a curve, relative to the sample, cross a ray: bits 0 and 8 say whether the
// first and the second count, from the signs of the curve's three coordinates across the ray
uint rootCode(float y1, float y2, float y3) {
    uint i1 = floatBitsToUint(y1) >> 31u;
    uint i2 = floatBitsToUint(y2) >> 30u;
    uint i3 = floatBitsToUint(y3) >> 29u;
    uint shift = (i2 & 2u) | (i1 & ~2u);
    shift = (i3 & 4u) | (shift & ~4u);
    return (0x2E74u >> shift) & 0x0101u;
}

// where a curve, relative to the sample, crosses y = 0, along x. Imaginary roots are a double
// root at the curve's turning point, and a nearly straight curve is solved as a line
vec2 crossHorizontal(vec4 p12, vec2 p3) {
    vec2 a = p12.xy - (p12.zw * 2.0) + p3;
    vec2 b = p12.xy - p12.zw;
    float d = sqrt(max((b.y * b.y) - (a.y * p12.y), 0.0));
    float t1 = (b.y - d) / a.y;
    float t2 = (b.y + d) / a.y;
    if (abs(a.y) < 1.0 / 65536.0) {
        t1 = p12.y * 0.5 / b.y;
        t2 = t1;
    }
    return vec2((((a.x * t1) - (b.x * 2.0)) * t1) + p12.x, (((a.x * t2) - (b.x * 2.0)) * t2) + p12.x);
}

// where a curve, relative to the sample, crosses x = 0, along y
vec2 crossVertical(vec4 p12, vec2 p3) {
    vec2 a = p12.xy - (p12.zw * 2.0) + p3;
    vec2 b = p12.xy - p12.zw;
    float d = sqrt(max((b.x * b.x) - (a.x * p12.x), 0.0));
    float t1 = (b.x - d) / a.x;
    float t2 = (b.x + d) / a.x;
    if (abs(a.x) < 1.0 / 65536.0) {
        t1 = p12.x * 0.5 / b.x;
        t2 = t1;
    }
    return vec2((((a.y * t1) - (b.y * 2.0)) * t1) + p12.y, (((a.y * t2) - (b.y * 2.0)) * t2) + p12.y);
}

// how much a winding number covers: any winding by the nonzero rule, and an odd one by the
// even-odd rule, each easing between whole windings as an edge crosses the pixel
float wound(float winding, bool evenOdd) {
    return evenOdd ? 1.0 - abs(1.0 - mod(abs(winding), 2.0)) : abs(winding);
}

/**
 * @param em the sample's place in the glyph, in em units with y upwards
 * @param glyph the value the glyph's bands start at
 * @param pixelsPerEm the em square's size in pixels along each axis, found where every sample of
 *        a quad reaches it, since a derivative is not defined inside a branch
 * @param evenOdd whether the glyph is filled by the even-odd rule rather than the nonzero rule
 * @return how much of the pixel the glyph covers
 **/
float curveCoverageAt(uint curves, uint bands, vec2 em, uint glyph, vec2 pixelsPerEm, bool evenOdd) {
    vec4 transform = vec4(bandValue(bands, glyph), bandValue(bands, glyph + 1u), bandValue(bands, glyph + 2u), bandValue(bands, glyph + 3u));
    ivec2 counts = ivec2(int(bandValue(bands, glyph + 4u)), int(bandValue(bands, glyph + 5u)));
    ivec2 band = clamp(ivec2(em * transform.xy + transform.zw), ivec2(0), max(counts - 1, ivec2(0)));

    // a ray cast along x, through the curves of the horizontal band the sample is in
    float xCoverage = 0.0;
    float xWeight = 0.0;
    uint header = glyph + 8u + (uint(band.y) * 2u);
    uint count = uint(bandValue(bands, header));
    uint list = uint(bandValue(bands, header + 1u));
    for (uint entry = 0u; entry < count; entry++) {
        uint curve = uint(bandValue(bands, list + entry));
        vec4 p12 = curveTexel(curves, curve * 2u) - vec4(em, em);
        vec2 p3 = curveTexel(curves, (curve * 2u) + 1u).xy - em;
        // sorted from the curve reaching furthest right, so one wholly left of the pixel ends it
        if (max(max(p12.x, p12.z), p3.x) * pixelsPerEm.x < -0.5) {
            break;
        }
        uint code = rootCode(p12.y, p12.w, p3.y);
        if (code != 0u) {
            vec2 r = crossHorizontal(p12, p3) * pixelsPerEm.x;
            if ((code & 1u) != 0u) {
                xCoverage += clamp(r.x + 0.5, 0.0, 1.0);
                xWeight = max(xWeight, clamp(1.0 - (abs(r.x) * 2.0), 0.0, 1.0));
            }
            if (code > 1u) {
                xCoverage -= clamp(r.y + 0.5, 0.0, 1.0);
                xWeight = max(xWeight, clamp(1.0 - (abs(r.y) * 2.0), 0.0, 1.0));
            }
        }
    }

    // and a ray cast along y, through the curves of the vertical band
    float yCoverage = 0.0;
    float yWeight = 0.0;
    header = glyph + 8u + (uint(counts.y) * 2u) + (uint(band.x) * 2u);
    count = uint(bandValue(bands, header));
    list = uint(bandValue(bands, header + 1u));
    for (uint entry = 0u; entry < count; entry++) {
        uint curve = uint(bandValue(bands, list + entry));
        vec4 p12 = curveTexel(curves, curve * 2u) - vec4(em, em);
        vec2 p3 = curveTexel(curves, (curve * 2u) + 1u).xy - em;
        if (max(max(p12.y, p12.w), p3.y) * pixelsPerEm.y < -0.5) {
            break;
        }
        uint code = rootCode(p12.x, p12.z, p3.x);
        if (code != 0u) {
            vec2 r = crossVertical(p12, p3) * pixelsPerEm.y;
            if ((code & 1u) != 0u) {
                yCoverage -= clamp(r.x + 0.5, 0.0, 1.0);
                yWeight = max(yWeight, clamp(1.0 - (abs(r.x) * 2.0), 0.0, 1.0));
            }
            if (code > 1u) {
                yCoverage += clamp(r.y + 0.5, 0.0, 1.0);
                yWeight = max(yWeight, clamp(1.0 - (abs(r.y) * 2.0), 0.0, 1.0));
            }
        }
    }

    // the two rays weighted by how near each came to an edge, and either winding direction counts
    float weighted = ((xCoverage * xWeight) + (yCoverage * yWeight)) / max(xWeight + yWeight, 1.0 / 65536.0);
    float coverage = max(wound(weighted, evenOdd), min(wound(xCoverage, evenOdd), wound(yCoverage, evenOdd)));
    return clamp(coverage, 0.0, 1.0);
}

/**
 * @return how much of the pixel a glyph covers, by the nonzero rule, the em square's size in
 *         pixels found from how fast em changes across the screen
 **/
float curveCoverage(uint curves, uint bands, vec2 em, uint glyph) {
    vec2 pixelsPerEm = 1.0 / max(fwidth(em), vec2(1.0 / 65536.0));
    return curveCoverageAt(curves, bands, em, glyph, pixelsPerEm, false);
}
