/* Moves the surface along its normal by a sine wave across x in shader space. */
displacement ripple(float height = 0.05; float frequency = 10) {
    point Pshader = ptransform("shader", P);
    P += height * sin(frequency * xcomp(Pshader)) * normalize(N);
    N = calculatenormal(P);
}
