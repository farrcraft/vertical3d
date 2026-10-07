// Tells a gate whether Node was asked to run it or a test imported it.
//
// Each gate runs its command line only when it is the file Node was started with, so a test can
// import the gate's functions without running the gate. The check compares the file Node was
// given with the gate's own path. It holds on every release of Node 24, where import.meta.main
// does not exist before 24.2.
//
// Node runs this file directly: it uses only node: modules and type annotations Node can strip.

import { resolve } from 'node:path';
import process from 'node:process';
import { fileURLToPath } from 'node:url';

/** Whether the module at url is the file Node was started with. */
export function isEntryPoint(url: string): boolean {
    const started = process.argv[1];
    return started !== undefined && resolve(started) === fileURLToPath(url);
}
