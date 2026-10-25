/*
 * A light along +z whose red is the "strength" the surface it lights passes it.
 */
light asker() {
    float strength = 0;
    surface("strength", strength);
    solar(vector (0, 0, 1), 0) {
        Cl = color(strength, 0, 0);
    }
}
