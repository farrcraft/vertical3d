// Keys whichever of a list of items the scene has, and lists the ones it lacks.
//
// A command the editor refuses throws an Error with the refusal as its message. Catching it lets
// the script go on, where an uncaught refusal stops the script at that line.

const wanted = ['Cube', 'Sphere'];
const missing: string[] = [];
for (const name of wanted) {
    try {
        timeline.set({ items: [name], frame: 1 });
    } catch (error) {
        missing.push(`${name} (${(error as Error).message})`);
    }
}
console.log('not keyed:', missing.join(', '));
