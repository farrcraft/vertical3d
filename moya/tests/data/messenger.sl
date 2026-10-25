/*
 * A white light along +z, which passes a tag to the surface it lights through an output
 * parameter.
 */
light messenger(output float tag = 0.5) {
    solar(vector (0, 0, 1), 0) {
        Cl = color(1, 1, 1);
    }
}
