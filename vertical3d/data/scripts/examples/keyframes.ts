// Keys the cube at every twelfth frame from 1 to 49, so that it can be animated between them.
//
// A script runs the editor's commands as functions. timeline.set keys the named items at a
// frame, as the timeline's Set Key does at the current one.

const first = 1;
const last = 49;
for (let frame = first; frame <= last; frame += 12) {
    timeline.set({ items: ['Cube'], frame });
}
console.log('keyed the cube from frame', first, 'to frame', last);
