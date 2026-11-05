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

/** One thing a gate reports: the file, the line when there is one, the rule, and what is wrong. */
export interface Finding {
    path: string;
    line: number | null;
    rule: string;
    message: string;
}

/** A finding as one line: "path:line: rule: message", or "path: rule: message" with no line. */
export function formatFinding({ path: file, line, rule, message }: Finding): string {
    return `${file}${line === null ? '' : `:${line}`}: ${rule}: ${message}`;
}

/**
 * Prints each finding on standard output, and returns the exit status: 1 when there is a
 * finding, 0 otherwise. After the findings comes one line that says what each breaks, written as
 * an error annotation that GitHub Actions shows on the run. each completes the sentence "N
 * findings above: each ...", in the singular.
 */
export function report(findings: readonly Finding[], each: string): number {
    for (const finding of findings) {
        process.stdout.write(formatFinding(finding) + '\n');
    }
    if (findings.length === 0) {
        return 0;
    }
    const count = findings.length === 1 ? '1 finding' : `${findings.length} findings`;
    process.stdout.write(`::error::${count} above: each ${each}\n`);
    return 1;
}

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
