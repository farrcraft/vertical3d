/*
 * Leaves the surface where it is, and passes a height to the surface shader through an output
 * parameter, as RI's message passing does.
 */
displacement lifted(output varying float height = 0) {
    height = 0.25;
}
