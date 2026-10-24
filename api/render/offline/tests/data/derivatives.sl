/*
 * Writes one of a shading point's derivatives into Ci, chosen by `which`: Du(P), Dv(P),
 * calculatenormal(P), or du and dv and Du(s).
 */
surface derivatives(float which = 0) {
    vector chosen = Du(P);
    if (which == 1) {
        chosen = Dv(P);
    }
    if (which == 2) {
        chosen = calculatenormal(P);
    }
    if (which == 3) {
        chosen = vector (du, dv, Du(s));
    }
    Oi = Os;
    Ci = color (xcomp(chosen), ycomp(chosen), zcomp(chosen));
}
