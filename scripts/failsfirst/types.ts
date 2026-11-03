/**
 * The words the fails-first harness uses for a change, a suite, a case and its result.
 */

export type Change = { status: string; file: string };
export type SuiteEntry = { suite: string; directory: string };
export type ListedSource = { suite: string; directory: string; source: string };
// A source dropped from a suite's list in the worktree, and whether the change added it.
export type Drop = { source: string; added: boolean };
export type Env = Record<string, string>;
export type Found = { path: string; stated: string | null; covers: string | null };

export type Case = {
    path: string;            // the --run_test path: suite names and the case name
    suite: string;           // the ctest suite, built as v3dtest_<suite>
    file: string;            // the test source, relative to the repository root
    stated: string | null;   // why the case passes without the change, from its doc comment
    claim: string | null;    // the text of a Covers the base: line, as written
    covers: string | null;   // the commit that line names, when it resolves and is in the base
    result: string | null;
    note: string;
};

// The result of a case, as the table prints it.
export const FAILS = "fails";
export const FAILS_TO_BUILD = "fails to build";
export const PASSES = "PASSES - weak";
export const PASSES_STATED = "passes, stated";
export const PASSES_COVERS = "passes, covers the base";
export const SKIPPED = "skipped";
export const NOT_RUN = "not run";
