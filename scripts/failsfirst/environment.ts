/**
 * The developer environment the worktree is built in, and running a command into a log.
 */

import { spawnSync } from 'node:child_process';
import * as fs from 'node:fs';
import * as path from 'node:path';

import { GateError } from '../lib/git.ts';
import { ROOT } from '../lib/tree.ts';
import type { Env } from './types.ts';

/** Enough for the output of a build or of a test binary. */
export const MAX_BUFFER = 512 * 1024 * 1024;

// The developer environment, as scripts\vcvars.cmd enters it, followed by a dump of the
// environment it leaves, which the build steps below are given.
const ENVIRONMENT_CMD = [
    '@echo off',
    `call "${path.join(ROOT, 'scripts', 'vcvars.cmd')}" || exit /b 1`,
    'set',
    '',
].join('\r\n');

export function readCache(build: string): Map<string, string> {
    const file = path.join(build, 'CMakeCache.txt');
    if (!fs.existsSync(file)) {
        throw new GateError(`${build} is not configured; the worktree build reuses its packages`);
    }
    const values = new Map<string, string>();
    for (const line of fs.readFileSync(file, 'utf8').split(/\r?\n/)) {
        const match = /^([A-Za-z_][\w-]*):[A-Z]+=(.*)$/.exec(line);
        if (match) {
            values.set(match[1], match[2]);
        }
    }
    return values;
}

export function developerEnvironment(scratch: string): Env {
    const script = path.join(scratch, 'environment.cmd');
    fs.writeFileSync(script, ENVIRONMENT_CMD);
    const done = spawnSync('cmd', ['/d', '/c', script], { encoding: 'utf8', maxBuffer: MAX_BUFFER });
    if (done.status !== 0) {
        throw new GateError('could not enter the developer environment:\n' + (done.stderr ?? '').trim());
    }
    const env: Env = {};
    for (const line of done.stdout.split(/\r?\n/)) {
        const at = line.indexOf('=');
        if (at > 0) {
            env[line.slice(0, at)] = line.slice(at + 1);
        }
    }
    return env;
}

export function runLogged(command: string[], log: string, env: Env): { code: number; output: string } {
    const fd = fs.openSync(log, 'w');
    let done;
    try {
        done = spawnSync(command[0], command.slice(1), { env, stdio: ['ignore', fd, fd] });
    } finally {
        fs.closeSync(fd);
    }
    return { code: done.status ?? 1, output: fs.readFileSync(log, 'utf8') };
}

export function tail(text: string, lines: number = 40): string {
    return text.split(/\r?\n/).slice(-lines).join('\n');
}
