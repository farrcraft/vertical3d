/*
 * Shows I, the incident direction, as the colour. Each component is written as it stands, so
 * a case can read I back off the grid.
 */
surface incident() {
    Ci = color(xcomp(I), ycomp(I), zcomp(I));
}
