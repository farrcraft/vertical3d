/**
 * How much of a pixel a distance field covers, as a glyph or as an effect drawn from one.
 *
 * A field's value is a distance from a glyph's edge, with the edge at 0.5 and the inside above
 * it. A pixel's width in field values is the length of the value's screen gradient, so an edge
 * stays one pixel wide at any scale and on a diagonal.
 *
 * A ramp of 0 draws the glyph: coverage rises across one pixel centred on its edge. Any other
 * ramp is an effect. Its low 16 bits are the value at which the effect is whole and its high 16
 * the value at which it is gone, each as a fraction of 65535. Coverage fades between the two,
 * and across one pixel centred between them when they are less than a pixel apart.
 **/

float fieldCoverage(float distance, uint ramp) {
    float pixel = max(length(vec2(dFdx(distance), dFdy(distance))), 1e-5);
    if (ramp == 0u) {
        return clamp(((distance - 0.5) / pixel) + 0.5, 0.0, 1.0);
    }
    float whole = float(ramp & 0xFFFFu) / 65535.0;
    float gone = float(ramp >> 16u) / 65535.0;
    if (whole - gone <= pixel) {
        return clamp(((distance - ((whole + gone) * 0.5)) / pixel) + 0.5, 0.0, 1.0);
    }
    return smoothstep(gone, whole, distance);
}
