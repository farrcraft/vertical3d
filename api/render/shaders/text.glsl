/**
 * How much of a pixel text covers once it is corrected to blend as it would in display space.
 *
 * A quad's contrast is below 0 for anything that is not text, which is left as it is. For text,
 * the contrast strengthens the coverage first, in every target: from 0, no change, to 1, which
 * lifts half coverage to three quarters.
 *
 * Built with ENCODED_TARGET, the target is _SRGB and blends in linear light, so the same
 * coverage would look thinner for dark text on a light ground and bolder for light text on a
 * dark ground. The coverage is then changed so that the linear blend gives the colour a
 * display space blend would, over a ground of the opposite luminance. Such a stage includes
 * srgb.glsl first.
 **/

/**
 * @param coverage how much of the pixel the glyph covers
 * @param shown the text's colour as it is displayed
 * @param contrast how much the coverage is strengthened, or below 0 for what is not text
 **/
float textCoverage(float coverage, vec3 shown, float contrast) {
    if (contrast < 0.0) {
        return coverage;
    }
    float corrected = coverage + (contrast * coverage * (1.0 - coverage));
#ifdef ENCODED_TARGET
    float text = dot(shown, vec3(0.2126, 0.7152, 0.0722));
    float ground = 1.0 - text;
    vec3 lit = linear(vec3(text, ground, mix(ground, text, corrected)));
    if (abs(lit.x - lit.y) > 1.0 / 1024.0) {
        corrected = (lit.z - lit.y) / (lit.x - lit.y);
    }
#endif
    return clamp(corrected, 0.0, 1.0);
}
