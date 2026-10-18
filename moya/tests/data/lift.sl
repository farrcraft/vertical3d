/*
 * Moves a surface along its normal by `amount`, and by `wave` times a sine of s on top, so a
 * flat surface comes out flat and lifted, or rippled. The normal is found again from the moved
 * points, so the surface shader sees the moved surface's normal.
 */
displacement lift(float amount = 0; float wave = 0; float frequency = 6.2831853) {
    P = P + (amount + wave * sin(frequency * s)) * normalize(N);
    N = calculatenormal(P);
}
