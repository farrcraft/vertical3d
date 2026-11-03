/**
 * The command line every review gate shares: when it runs, how it reads its options, and how it
 * ends.
 *
 * A gate exits with status 0 when it finds nothing, 1 when it reports a finding, and 2 when it
 * cannot run. A gate that cannot run prints one line saying why, on standard error.
 */

import * as path from 'node:path';
import process from 'node:process';
import { fileURLToPath } from 'node:url';
import { parseArgs, type ParseArgsOptionsConfig } from 'node:util';

import { shouldRun } from './entry.ts';
import { GateError } from './git.ts';

/** Options a gate was not given correctly. The gate prints its usage and exits with status 2. */
export class UsageError extends GateError {}

/**
 * Runs a gate's main function when Node was started with the gate's file, and sets the exit
 * status from it. A GateError ends the gate with status 2. Any other error is a defect, and
 * Node reports it with its stack.
 */
export function runGate(url: string, main: () => number): void {
    if (!shouldRun(url)) {
        return;
    }
    try {
        process.exitCode = main();
    } catch (error) {
        if (!(error instanceof GateError)) {
            throw error;
        }
        const name = path.basename(fileURLToPath(url));
        process.stderr.write(error instanceof UsageError ? `${error.message}\n` : `${name}: ${error.message}\n`);
        process.exitCode = 2;
    }
}

/**
 * The options and positional arguments of the command line, read against a gate's options.
 * Every option has to be one the gate takes, with a value when it needs one, and an option that
 * takes a value refuses an empty one. Anything else throws a UsageError carrying the usage.
 */
export function options<T extends ParseArgsOptionsConfig>(usage: string, config: T, positionals = false,
    args: string[] = process.argv.slice(2)) {
    let parsed;
    try {
        parsed = parseArgs({ args, options: config, strict: true, allowPositionals: positionals });
    } catch {
        throw new UsageError(usage);
    }
    for (const value of Object.values(parsed.values)) {
        if (typeof value === 'string' && value.trim() === '') {
            throw new UsageError(usage);
        }
    }
    return parsed;
}
