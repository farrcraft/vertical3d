/**
 * Reading and editing the v3d_add_test lists of a tests/CMakeLists.txt, and reading what a
 * failed configure reports.
 */

import * as fs from "node:fs";
import * as path from "node:path";

import { isTestPath } from "../lib/tree.ts";
import { git, show } from "./repo.ts";
import type { ListedSource, SuiteEntry } from "./types.ts";

const ADD_TEST = /v3d_add_test\s*\(\s*(\w+)([\s\S]*?)\)/g;

// One entry of a v3d_add_test list: a quoted string, or a word.
const LIST_TOKEN = /"([^"]+)"|([^\s()"]+)/g;

// The file a v3d_add_test entry in <directory>/CMakeLists.txt names, relative to the repository
// root, or null for an entry that depends on a variable other than CMAKE_CURRENT_SOURCE_DIR.
export function sourceOf(directory: string, entry: string): string | null {
    const spelled = entry.replace(/^\$\{CMAKE_CURRENT_SOURCE_DIR\}\//, "");
    if (spelled.includes("$")) {
        return null;
    }
    return path.posix.normalize(path.posix.join(directory, spelled));
}

// Every source named by a v3d_add_test list at the revision, with its suite and the directory
// of the tests/CMakeLists.txt that names it. A source two suites name appears once for each.
export function listedSources(revision: string): ListedSource[] {
    const listed: ListedSource[] = [];
    const files = git(["ls-tree", "-r", "-z", "--name-only", revision]).stdout.split("\0");
    for (const file of files) {
        if (path.posix.basename(file) !== "CMakeLists.txt" || !isTestPath(file)) {
            continue;
        }
        const text = (show(revision, file) ?? "").replace(/#[^\n]*/g, "");
        const directory = path.posix.dirname(file);
        for (const match of text.matchAll(ADD_TEST)) {
            const suite = match[1];
            for (const token of match[2].matchAll(LIST_TOKEN)) {
                const source = sourceOf(directory, token[1] ?? token[2]);
                if (source !== null) {
                    listed.push({ suite, directory, source });
                }
            }
        }
    }
    return listed;
}

// Every test source named by a v3d_add_test list at the revision, with its suite.
export function suiteMap(revision: string): Map<string, SuiteEntry> {
    const mapping = new Map<string, SuiteEntry>();
    for (const { suite, directory, source } of listedSources(revision)) {
        mapping.set(source, { suite, directory });
    }
    return mapping;
}

// Removes a source from the v3d_add_test(<suite> ...) list in the worktree's
// <directory>/CMakeLists.txt, whether the list quotes it or not. Returns whether it was there.
export function dropSource(worktree: string, directory: string, suite: string, source: string): boolean {
    const file = path.join(worktree, directory, "CMakeLists.txt");
    if (!fs.existsSync(file)) {
        return false;
    }
    const content = fs.readFileSync(file, "utf8");
    // comments are blanked to spaces, so an offset in the copy is the same offset in the file
    const uncommented = content.replace(/#[^\n]*/g, (comment) => " ".repeat(comment.length));
    const call = new RegExp(`v3d_add_test\\s*\\(\\s*${escapeRegExp(suite)}\\b([\\s\\S]*?)\\)`).exec(uncommented);
    if (!call) {
        return false;
    }
    const listStart = call.index + call[0].length - call[1].length - 1;
    for (const token of call[1].matchAll(LIST_TOKEN)) {
        if (sourceOf(directory, token[1] ?? token[2]) === source) {
            const at = listStart + token.index;
            fs.writeFileSync(file, content.slice(0, at) + content.slice(at + token[0].length));
            return true;
        }
    }
    return false;
}

// The sources a failed configure reports it cannot find, as the CMakeLists.txt spelled them.
export function missingSources(output: string): string[] {
    return [...output.matchAll(/Cannot find source file:\s*\r?\n\s*(\S[^\r\n]*)/g)].map((m) => m[1].trim());
}

// A call a failed configure reports to an unknown command: the file as the configure printed it,
// the line the call starts on, and the command.
export type UnknownCall = { file: string; line: number; command: string };

export function unknownCommands(output: string): UnknownCall[] {
    const calls = output.matchAll(/CMake Error at (\S[^\r\n]*?):(\d+) \((\w+)\):\s*\r?\n\s*Unknown CMake command "\3"/g);
    return [...calls].map((m) => ({ file: m[1], line: Number(m[2]), command: m[3] }));
}

// The text with the call to command that starts on the given line removed, up to its closing
// parenthesis, or null when that line has no such call. CMake command names ignore case.
export function dropCall(text: string, line: number, command: string): string | null {
    let start = 0;
    for (let n = 1; n < line; n++) {
        start = text.indexOf("\n", start) + 1;
        if (start === 0) {
            return null;
        }
    }
    const end = text.indexOf("\n", start);
    const found = new RegExp(`\\b${escapeRegExp(command)}\\s*\\(`, "i").exec(text.slice(start, end === -1 ? undefined : end));
    if (!found) {
        return null;
    }
    let depth = 0;
    let quoted = false;
    for (let i = start + found.index + found[0].length - 1; i < text.length; i++) {
        const c = text[i];
        if (quoted) {
            if (c === "\\") {
                i++;
            } else if (c === "\"") {
                quoted = false;
            }
        } else if (c === "\"") {
            quoted = true;
        } else if (c === "(") {
            depth++;
        } else if (c === ")" && --depth === 0) {
            return text.slice(0, start + found.index) + text.slice(i + 1);
        }
    }
    return null;
}

function escapeRegExp(text: string): string {
    return text.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}
