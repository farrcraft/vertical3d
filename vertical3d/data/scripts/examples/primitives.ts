// Makes one of each primitive, then takes the last one back.
//
// Each command is one step of the edit history, as it would be from the Create menu, so
// edit.undo removes the cone and leaves the rest.

const makers: Array<() => void> = [create.poly.plane, create.poly.cylinder, create.poly.cone];
for (const make of makers) {
    make();
}
edit.undo();
