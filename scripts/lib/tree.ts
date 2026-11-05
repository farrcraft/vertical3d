/**
 * The repository the review gates check: its root, and which of its paths are not this tree's
 * own code.
 */

import * as path from 'node:path';

/** The repository root. Every path a gate reports is relative to it, with forward slashes. */
export const ROOT = path.resolve(import.meta.dirname, '..', '..');

/** The directories of third-party code. The tree's rules do not cover them. */
export const THIRD_PARTY: readonly string[] = ['vendor/', 'ports/'];

/** The directories a build or an install writes. Nothing in them is a source. */
export const GENERATED: readonly string[] = ['out/', 'node_modules/', 'vcpkg_installed/'];

/** Whether a path is in a directory of third-party code. */
export function isThirdParty(file: string): boolean {
    return THIRD_PARTY.some((prefix) => file.startsWith(prefix));
}

/** Whether a path is in a directory a build or an install writes, at the root or below it. */
export function isGenerated(file: string): boolean {
    return GENERATED.some((prefix) => file.startsWith(prefix)) || file.includes('/vcpkg_installed/');
}

/** Whether a path is under a directory named tests, at any depth. */
export function isTestPath(file: string): boolean {
    return /(^|\/)tests\//.test(file);
}

