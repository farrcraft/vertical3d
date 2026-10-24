/*
 * A surface bumped along its normal by a sine of s, which writes the bumped normal into Ci.
 * Each component is moved from [-1, 1] onto [0, 1], so a negative one survives the grid.
 */
surface bumpnormal(float amplitude = 0.1; float frequency = 6.2831853) {
    normal Nn = normalize(N);
    point Pb = P + amplitude * sin(frequency * s) * Nn;
    normal Nb = normalize(calculatenormal(Pb));
    Oi = Os;
    Ci = 0.5 + 0.5 * color (xcomp(Nb), ycomp(Nb), zcomp(Nb));
}
