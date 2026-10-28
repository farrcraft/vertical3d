/*
 * Shows N turned by two-argument faceforward as the colour, so a case can read back which way
 * the normal was turned.
 */
surface faceforwarded() {
    normal turned = faceforward(N, I);
    Ci = color(xcomp(turned), ycomp(turned), zcomp(turned));
}
