/*
 * Shows the "tag" each light passed as red.
 */
surface heard() {
    float tag = 0;
    illuminance(P, N, 1.5707963) {
        lightsource("tag", tag);
    }
    Ci = color(tag, 0, 0);
}
