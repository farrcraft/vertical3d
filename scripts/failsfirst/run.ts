/**
 * Running one case alone, and reading how its run ended.
 */

import { spawnSync } from 'node:child_process';
import * as path from 'node:path';

import { MAX_BUFFER } from './environment.ts';
import { type Case, type Env, FAILS, NOT_RUN, PASSES, PASSES_COVERS, PASSES_STATED, SKIPPED } from './types.ts';

// A Boost.Test executable that found no usable device, or no window, before starting. Only these
// suites return it.
const SKIP_RETURN_CODE = 77;
const SKIPPING_SUITES = new Set(['render_device', 'render_window']);

// What Boost.Test prints when a check, a requirement or a whole case fails, including a case
// that threw or crashed inside the framework.
const BOOST_FAILURE = /\*\*\* \d+ failures? (?:is|are) detected|\berror: in "|\bfailure occurred/i;

// An exit status as Windows reports it: an NTSTATUS such as 0xC0000135 in hexadecimal.
export function describeExit(status: number | null, signal: string | null): string {
    if (signal !== null) {
        return `signal ${signal}`;
    }
    if (status === null) {
        return 'no status';
    }
    // an NTSTATUS read as a signed number is negative
    return status > 0xFFFF || status < 0 ? `0x${(status >>> 0).toString(16).toUpperCase()}` : String(status);
}

export type Outcome = { error: Error | undefined; status: number | null; signal: string | null; output: string };

export function runCase(c: Case, executable: string, env: Env, timeout: number): void {
    const done = spawnSync(executable, [`--run_test=${c.path}`, '--detect_memory_leaks=0'], {
        cwd: path.dirname(executable), env, encoding: 'utf8', maxBuffer: MAX_BUFFER, timeout: timeout * 1000,
    });
    const output = (done.stdout ?? '') + (done.stderr ?? '');
    classify(c, { error: done.error, status: done.status, signal: done.signal, output }, timeout);
}

// Sets the case's result from how its run ended. Only a failure Boost.Test reported counts as
// failing.
export function classify(c: Case, done: Outcome, timeout: number): void {
    if (done.error) {
        c.result = NOT_RUN;
        const code = (done.error as NodeJS.ErrnoException).code;
        c.note = code === 'ETIMEDOUT' ? `timed out after ${timeout}s` : `could not start: ${done.error.message}`;
        return;
    }
    const output = done.output;
    if (done.status === 0) {
        if (c.stated !== null) {
            c.result = PASSES_STATED;
        } else if (c.covers !== null) {
            c.result = PASSES_COVERS;
        } else {
            c.result = PASSES;
            if (c.claim !== null) {
                c.note = 'its Covers the base: line names no commit the base contains';
            }
        }
    } else if (done.status === SKIP_RETURN_CODE && SKIPPING_SUITES.has(c.suite)) {
        c.result = SKIPPED;
    } else if (/no test cases matching filter|test setup error/i.test(output)) {
        c.result = NOT_RUN;
        c.note = 'the binary has no such case';
    } else if (BOOST_FAILURE.test(output)) {
        c.result = FAILS;
    } else {
        // a binary that cannot load a DLL, or that dies before the framework reports, has not
        // shown that the case fails
        c.result = NOT_RUN;
        c.note = `exited with ${describeExit(done.status, done.signal)} and reported no failed check`;
    }
}
