/*
 * Shows the "height" the displacement shader passed as red, or blue when none was passed.
 */
surface heightshown() {
    float height = 0;
    if (displacement("height", height) != 0) {
        Ci = color(height, 0, 0);
    } else {
        Ci = color(0, 0, 1);
    }
}
