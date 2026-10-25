/*
 * Shows the shading point in object space as the colour, so a case can read back where the
 * point is in the space of the primitive it lies on.
 */
surface objectspace() {
    point here = ptransform("object", P);
    Ci = color(xcomp(here), ycomp(here), zcomp(here));
}
