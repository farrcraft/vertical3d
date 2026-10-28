/*
 * A distant light straight down -z whose colour reads a derivative of the point it lights,
 * so that a surface it shines on is shaded with its neighbours. The derivative is taken
 * times zero, so the light lights as a distant light does.
 */
light stepped(float intensity = 1) {
    solar(vector (0, 0, -1), 0) {
        Cl = intensity * (1 + 0 * area(Ps));
    }
}
