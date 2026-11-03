/**
 * Configuring the worktree and building the affected suites, and telling which sources made a
 * build fail.
 */

import * as fs from 'node:fs';
import * as path from 'node:path';

import { GateError } from '../lib/git.ts';
import { ROOT, isTestPath } from '../lib/tree.ts';
import { dropCall, dropSource, listedSources, missingSources, sourceOf, unknownCommands } from './cmake.ts';
import { runLogged, tail } from './environment.ts';
import { type Case, type Drop, type Env, FAILS_TO_BUILD, NOT_RUN } from './types.ts';

// Configures the worktree. A source the configure cannot find is dropped from each head list that
// names it and resolves to a file the worktree lacks, and the configure runs again. A list in
// another directory that spells the name the same way names a different file, and keeps it. The
// drops are added to dropped, marked with whether the change added the source. A call in a
// tests/ CMakeLists.txt that the configure reports as an unknown command is removed, and the
// configure runs again.
export function configure(worktree: string, build: string, mainBuild: string, cache: Map<string, string>, env: Env,
                   scratch: string, head: string, added: Set<string>, dropped: Map<string, Drop[]>): void {
    let toolchain = cache.get('CMAKE_TOOLCHAIN_FILE') ?? 'vendor/vcpkg/scripts/buildsystems/vcpkg.cmake';
    if (!path.isAbsolute(toolchain)) {
        toolchain = path.join(ROOT, toolchain);
    }
    const installed = cache.get('VCPKG_INSTALLED_DIR') || path.join(mainBuild, 'vcpkg_installed');
    const command = [
        'cmake', '-S', worktree, '-B', build, '-G', 'Ninja',
        '-DCMAKE_BUILD_TYPE=' + (cache.get('CMAKE_BUILD_TYPE') ?? 'Debug'),
        '-DCMAKE_TOOLCHAIN_FILE=' + toolchain.replaceAll('\\', '/'),
        '-DVCPKG_TARGET_TRIPLET=' + (cache.get('VCPKG_TARGET_TRIPLET') ?? 'x64-windows'),
        '-DVCPKG_INSTALLED_DIR=' + installed.replaceAll('\\', '/'),
        '-DVCPKG_MANIFEST_INSTALL=OFF',
        '-DV3D_WARNINGS_AS_ERRORS=OFF',
    ];
    // a link edge also copies dlls out of the shared install, so a build that serialised links
    // for that reason has them serialised here too
    for (const name of ['CMAKE_JOB_POOLS', 'CMAKE_JOB_POOL_LINK']) {
        const value = cache.get(name);
        if (value) {
            command.push(`-D${name}=${value}`);
        }
    }
    const listed = listedSources(head);
    for (let attempt = 1; ; attempt++) {
        console.log(`configuring ${build}`);
        const { code, output } = runLogged(command, path.join(scratch, `configure-${attempt}.log`), env);
        if (code === 0) {
            return;
        }
        let progress = false;
        for (const name of missingSources(output)) {
            const named = name.replaceAll('\\', '/');
            for (const { suite, directory, source } of listed) {
                // the configure prints the entry as the list spelled it, or as an absolute path
                const full = path.isAbsolute(name) ?
                    path.relative(worktree, name).split(path.sep).join('/') : sourceOf(directory, named);
                if (full !== source || fs.existsSync(path.join(worktree, ...source.split('/')))) {
                    continue;
                }
                if (dropSource(worktree, directory, suite, source)) {
                    console.log(`dropped ${source} from v3dtest_${suite}: the configure cannot find it`);
                    dropped.set(suite, [...(dropped.get(suite) ?? []), { source, added: added.has(source) }]);
                    progress = true;
                }
            }
        }
        // a head tests/CMakeLists.txt can call a helper the change added outside tests/
        for (const { file, line, command: name } of unknownCommands(output)) {
            const relative = (path.isAbsolute(file) ? path.relative(worktree, file) : file).split(path.sep).join('/');
            if (!isTestPath(relative)) {
                continue;
            }
            const full = path.join(worktree, ...relative.split('/'));
            const text = fs.existsSync(full) ? dropCall(fs.readFileSync(full, 'utf8'), line, name) : null;
            if (text !== null) {
                fs.writeFileSync(full, text);
                console.log(`dropped the call to ${name} from ${relative}: the base has no such command`);
                progress = true;
            }
        }
        if (!progress || attempt >= 10) {
            throw new GateError('the worktree did not configure:\n' + tail(output));
        }
    }
}

// The suite's own sources that failed to compile, or whose objects name a symbol the link of the
// suite could not find. Null when anything else failed, or a failure names no source of the suite.
export function failedSources(output: string, directory: string, suite: string, sources: string[]): string[] | null {
    const failed: string[] = [];
    const marker = `CMakeFiles/v3dtest_${suite}.dir/`;
    const executable = `v3dtest_${suite}.exe`;
    let linked = false;
    for (const match of output.matchAll(/^FAILED: (?:\[code=\d+\] )?(\S+)/gm)) {
        const outputFile = (match[1] ?? '').replaceAll('\\', '/');
        if (path.posix.basename(outputFile) === executable) {
            linked = true;
            continue;
        }
        if (!outputFile.includes(marker) || !outputFile.endsWith('.obj')) {
            return null;
        }
        const relative = (outputFile.split(marker)[1] ?? '').slice(0, -'.obj'.length)
            .split('/').map((part) => (part === '__' ? '..' : part)).join('/');
        failed.push(path.posix.normalize(path.posix.join(directory, relative)));
    }
    if (!linked) {
        return failed;
    }
    // an unresolved symbol names the object that refers to it; any other link error is not a
    // source's
    if (/error LNK(?!2001|2019|1120)\d+/.test(output)) {
        return null;
    }
    const objects = new Set<string>();
    for (const match of output.matchAll(/^\s*(\S[^\r\n]*?\.obj) : error LNK(?:2001|2019)/gim)) {
        objects.add(path.posix.basename((match[1] ?? '').replaceAll('\\', '/')).toLowerCase());
    }
    if (objects.size === 0) {
        return null;
    }
    for (const object of objects) {
        const owners = sources.filter((s) => (path.posix.basename(s) + '.obj').toLowerCase() === object);
        const [owner] = owners;
        if (owners.length !== 1 || owner === undefined) {
            return null;
        }
        failed.push(owner);
    }
    return failed;
}

// Why a suite that lost sources from its list may not build: the sources the change added, and
// those the worktree lacks for another reason.
export function dropNote(drops: Drop[]): string {
    const added = drops.filter((d) => d.added).map((d) => d.source);
    const missing = drops.filter((d) => !d.added).map((d) => d.source);
    const parts: string[] = [];
    if (added.length > 0) {
        parts.push(`the suite needs ${added.join(', ')}, which the change added`);
    }
    if (missing.length > 0) {
        parts.push(`the suite lists ${missing.join(', ')}, which the worktree does not have`);
    }
    return parts.join('; ');
}

function markUnbuilt(cases: Case[]): void {
    for (const c of cases) {
        if (c.result === null) {
            c.result = FAILS_TO_BUILD;
        }
    }
}

// Builds every affected suite in one ninja run, before buildSuite builds each in turn. One run
// keeps every core busy across the suites, where building them one after another leaves the
// machine idle while each one's last edges finish. Its result is not read: buildSuite finds a
// suite that built here already up to date, and builds and diagnoses one that did not. A run
// that names a target missing from the build fails at once, so a suite the base lacks is left out.
export function buildAll(suites: string[], build: string, env: Env, scratch: string): void {
    if (suites.length < 2) {
        return;
    }
    const listing = runLogged(['ninja', '-C', build, '-t', 'targets', 'all'], path.join(scratch, 'targets.log'), env);
    if (listing.code !== 0) {
        return;
    }
    const known = new Set(listing.output.split(/\r?\n/).map((line) => (line.split(':')[0] ?? '').trim()));
    const targets = suites.map((s) => 'v3dtest_' + s).filter((t) => known.has(t));
    if (targets.length < 2) {
        return;
    }
    console.log(`building ${targets.join(', ')}`);
    runLogged(['ninja', '-k', '0', '-C', build, ...targets], path.join(scratch, 'build-all.log'), env);
}

// Builds v3dtest_<suite>. Marks the cases that cannot be built or run, and returns the
// executable, or null when there is none to run.
export function buildSuite(suite: string, directory: string, sources: string[], cases: Case[], worktree: string,
                    build: string, env: Env, scratch: string): string | null {
    const target = 'v3dtest_' + suite;
    const dropped = new Set<string>();
    for (;;) {
        console.log(`building ${target}`);
        const { code, output } = runLogged(['ninja', '-k', '0', '-C', build, target],
                                           path.join(scratch, `build-${suite}.log`), env);
        if (code === 0) {
            break;
        }
        if (output.includes('unknown target')) {
            for (const c of cases) {
                c.result = FAILS_TO_BUILD;
                c.note = 'the suite does not exist without the change';
            }
            return null;
        }
        // a test file that does not build is dropped whether or not it has new cases: one with
        // none loses nothing, and the rest of the suite can still run
        const failed = failedSources(output, directory, suite, sources);
        if (!failed || failed.length === 0 || failed.some((f) => !isTestPath(f) || dropped.has(f))) {
            console.log(tail(output, 20));
            markUnbuilt(cases);
            return null;
        }
        for (const source of failed) {
            if (!dropSource(worktree, directory, suite, source)) {
                markUnbuilt(cases);
                return null;
            }
            dropped.add(source);
            for (const c of cases) {
                if (c.file === source) {
                    c.result = FAILS_TO_BUILD;
                    c.note = 'its file does not compile or link';
                }
            }
        }
        if (cases.every((c) => c.result !== null)) {
            return null;
        }
    }
    const executable = path.join(build, ...directory.split('/'), target + '.exe');
    if (fs.existsSync(executable)) {
        return executable;
    }
    for (const entry of fs.readdirSync(build, { recursive: true }) as string[]) {
        if (path.basename(entry) === target + '.exe') {
            return path.join(build, entry);
        }
    }
    for (const c of cases) {
        if (c.result === null) {
            c.result = NOT_RUN;
            c.note = `the build made no ${target}.exe`;
        }
    }
    return null;
}
