/*
 * A surface that is its own colour plus whatever it sees along its normal. Two of them
 * facing each other trace back and forth until the scene's trace depth stops them, and
 * each level adds its own Cs, so the sum says how deep the trace went and whether each
 * level kept its own registers.
 */
surface relay() {
    Ci = Cs + trace(P, N);
}
