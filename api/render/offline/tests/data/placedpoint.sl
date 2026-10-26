/*
 * Writes where a point is in object space and in shader space into Ci, and adds the light
 * falling on it, so that the colour depends on everything a run of the shader holds uniform.
 */
surface placedpoint() {
    point inObject = ptransform("object", P);
    point inShader = ptransform("shader", P);
    color fromObject = color (xcomp(inObject), ycomp(inObject), zcomp(inObject));
    color fromShader = color (xcomp(inShader), ycomp(inShader), zcomp(inShader));
    normal Nf = faceforward(normalize(N), I);
    Oi = Os;
    Ci = fromObject + 0.25 * fromShader + diffuse(Nf);
}
