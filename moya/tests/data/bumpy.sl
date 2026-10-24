/*
 * A matte surface bumped along its normal by noise in its own space. The noise is a fixed
 * function of position, so the bumps are the same on every render.
 */
surface bumpy(float Ka = 1; float Kd = 1; float amplitude = 0.08; float frequency = 2.5) {
    normal Nn = normalize(N);
    point Pb = P + amplitude * noise(ptransform("shader", P) * frequency) * Nn;
    normal Nf = faceforward(normalize(calculatenormal(Pb)), I);
    Oi = Os;
    Ci = Os * Cs * (Ka * ambient() + Kd * diffuse(Nf));
}
