/*
 * A surface that never finishes. The machine stops it at its instruction limit and the run
 * fails, which is how a test reaches the path a renderer takes when a shader fails.
 */
surface endless() {
    float i = 0;
    while (i >= 0) {
        i += 1;
    }
    Ci = Cs;
}
