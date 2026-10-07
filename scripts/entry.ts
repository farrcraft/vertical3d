// Decides whether a gate runs its command line.
//
// A gate runs only when it is the file Node was started with, so a test can import its functions
// without running it. Node resolves a module's own path through symbolic links and junctions,
// and leaves the path it was started with as given, so both are resolved to the real file before
// they are compared. import.meta.main is not used, because it does not exist before Node 24.2.
//
// The gates support Node 24 and later. A gate started on an earlier Node stops with exit 2 and a
// message, rather than running on a Node it was not written for. A Node too old to strip types
// fails to load the file at all, which is a failure too.
//
// Node runs this file directly: it uses only node: modules and type annotations Node can strip.

import { realpathSync } from 'node:fs';
import { resolve } from 'node:path';
import process from 'node:process';
import { fileURLToPath } from 'node:url';

/** The earliest major release of Node the gates support. */
export const SUPPORTED_NODE = 24;

/** Whether a Node version string, such as "24.19.0", is one the gates support. */
export function supportedNode(version: string): boolean {
    const major = Number.parseInt(version.split('.')[0], 10);
    return Number.isInteger(major) && major >= SUPPORTED_NODE;
}

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

/**
 * Whether the gate at url runs its command line: true when Node was started with it on a
 * supported Node. On an earlier Node it writes why and stops the process with exit 2.
 */
export function shouldRun(url: string): boolean {
    if (!isEntryPoint(url)) {
        return false;
    }
    if (!supportedNode(process.versions.node)) {
        process.stderr.write(`the review gates need Node ${SUPPORTED_NODE} or later, and this is Node ${process.versions.node}\n`);
        process.exit(2);
    }
    return true;
}
