/**
 * Check the linking rules for apps and test suites.
 *
 * The script reads every CMakeLists.txt in the tree and checks each target that is not an api
 * library: every app, every app library such as v3dlib_moya, every app test suite and every api
 * test suite. A target's files are the sources its add_executable, add_library or v3d_add_test
 * call lists, and every header those include that is neither in api/ nor in an app library's
 * directory, followed transitively. A suite's files therefore include the ../src files it
 * compiles and the app headers those include. A header under a tests/ directory is a local
 * header, even under api/.
 *
 *   missing-link      a file includes the header of a v3dlib_* target that the target does
 *                     not name.
 *   extra-link        the target names a v3dlib_* target whose headers none of its files
 *                     include.
 *   namespace-use     a file names something in a v3d::<lib>:: namespace, and none of the
 *                     target's files includes a header of that library. The type may arrive
 *                     through another header; the rule is that a target includes what it uses.
 *   visibility        an app library's header includes a v3dlib_* target that the app library
 *                     links PRIVATE, or the app library links one PUBLIC that none of its
 *                     headers includes. A header under an internal/ directory is not counted,
 *                     because only the library's own sources include it.
 *   package           an app or an app suite names a third-party package that is not the
 *                     exception. The exception is a package its own files include directly
 *                     that none of the v3dlib_* targets it links carries. A suite never names
 *                     Boost::unit_test_framework, because v3d_add_test links it. A variable
 *                     such as ${Boost_LIBRARIES} names every target it expands to: config-mode
 *                     Boost sets it to all the components the tree finds, and
 *                     ${PNG_LIBRARIES} and its kin name the package's target.
 *   package-unlinked  an app or an app suite includes a package header that no target it
 *                     links carries and that it does not name itself.
 *
 * Api libraries are not checked here. The configure checks them against the manifest in
 * cmake/v3dApiLibraries.cmake: what each one links, and whether a link is PUBLIC or PRIVATE.
 * examples/ is not checked either. It is a project outside the tree's build, and CI builds it
 * separately.
 *
 * Includes are found by reading the text, after comments and #if 0 blocks are removed. A
 * conditional include is counted whether or not its condition holds.
 *
 * Usage: node scripts/linkrule.ts
 *
 * Each violation prints as "path: rule: detail". The exit status is 1 when there is a
 * violation, 2 when the command is given an argument, and 0 otherwise.
 */

import { options, runGate } from './lib/cli.ts';
import { tracked } from './lib/git.ts';
import { ROOT } from './lib/tree.ts';
import { type Report, check } from './linkrule/rules.ts';
import { Tree } from './linkrule/scan.ts';
import { type Target, loadTargets } from './linkrule/targets.ts';

export { includesIn, usesIn } from './linkrule/cpp.ts';

/**
 * The violations in a tree, each as "path: rule: detail": every target its CMake files define
 * that is not an api library, checked. The files are relative to the root.
 */
export function violations(root: string, cmakeFiles: readonly string[], apiFiles: readonly string[]): string[] {
    const { targets, boost } = loadTargets(root, cmakeFiles);
    const tree = new Tree(root, targets, apiFiles);
    const found: string[] = [];
    const report: Report = (where, rule, detail) => {
        found.push(`${where}: ${rule}: ${detail}`);
    };
    for (const name of [...targets.keys()].sort()) {
        const t = targets.get(name) as Target;
        if (t.kind !== 'api') {
            check(tree, t, boost, report);
        }
    }
    return found;
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    options('usage: node scripts/linkrule.ts', {});
    // examples/ is a project outside the tree's build
    const cmake = tracked('*CMakeLists.txt', '*.cmake').filter((f) => !f.startsWith('examples/'));
    const found = violations(ROOT, cmake, tracked('api'));
    for (const line of found) {
        console.log(line);
    }
    return found.length > 0 ? 1 : 0;
}

runGate(import.meta.url, main);
