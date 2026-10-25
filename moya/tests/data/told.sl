/*
 * Sums the light reaching it. Its output parameter "strength" is read by a light that asks
 * the surface for it.
 */
surface told(output float strength = 0.75) {
    Ci = 0;
    illuminance(P, N, 1.5707963) {
        Ci += Cl;
    }
}
