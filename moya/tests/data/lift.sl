/*
 * Moves a surface along its normal by `amount`, and by `wave` times a sine of x in the shader's
 * own space on top, so a flat surface comes out flat and lifted, or rippled. The ripple follows
 * position rather than s, because a polygon split into pieces gives each piece's grid its own s.
 * The normal is found again from the moved points, so the surface shader sees the moved
 * surface's normal.
 */
displacement lift(float amount = 0; float wave = 0; float frequency = 6.2831853) {
    float across = xcomp(ptransform("shader", P));
    P = P + (amount + wave * sin(frequency * across)) * normalize(N);
    N = calculatenormal(P);
}
