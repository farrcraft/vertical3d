/*
 * A matte surface checked in squares of `size` across the x and z axes of current space,
 * between its own colour and `other`. It is the floor a reflection or a refraction is easy
 * to read against: a bent ray shows as a bent line.
 */
surface checked(float Ka = 1; float Kd = 1; float size = 1; color other = color (0.15, 0.15, 0.15)) {
    normal Nf = faceforward(normalize(N), I);
    float which = mod(floor(xcomp(P) / size) + floor(zcomp(P) / size), 2);
    Oi = Os;
    Ci = Os * mix(Cs, other, which) * (Ka * ambient() + Kd * diffuse(Nf));
}
