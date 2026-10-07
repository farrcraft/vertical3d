// Tells a gate whether Node was asked to run it or a test imported it.
//
// Each gate runs its command line only when it is the file Node was started with, so a test can
// import the gate's functions without running the gate. Node resolves a module's own path
// through symbolic links and junctions, and leaves the path it was started with as given, so
// both are resolved to the real file before they are compared. The check holds on every release
// of Node 24, where import.meta.main does not exist before 24.2.
//
// Node runs this file directly: it uses only node: modules and type annotations Node can strip.

import { realpathSync } from 'node:fs';
import { resolve } from 'node:path';
import process from 'node:process';
import { fileURLToPath } from 'node:url';

/** The real path of a file, or null when it cannot be resolved. */
function real(file: string): string | null {
    try {
        return realpathSync(resolve(file));
    } catch {
        return null;
    }
}

/** Whether the module at url is the file Node was started with. */
export function isEntryPoint(url: string): boolean {
    const started = process.argv[1];
    if (started === undefined) {
        return false;
    }
    const module = real(fileURLToPath(url));
    return module !== null && real(started) === module;
}
