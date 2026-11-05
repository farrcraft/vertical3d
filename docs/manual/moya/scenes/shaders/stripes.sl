/* Stripes of two colours along s, lit like matte. */
surface stripes(float Ka = 1; float Kd = 1; float frequency = 8;
        color stripecolor = color (1, 1, 1)) {
    normal Nf = faceforward(normalize(N), I);
    float band = step(0.5, mod(s * frequency, 1));
    color base = mix(Cs, stripecolor, band);
    Oi = Os;
    Ci = Os * base * (Ka * ambient() + Kd * diffuse(Nf));
}
