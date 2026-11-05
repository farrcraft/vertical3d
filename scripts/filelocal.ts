/**
 * Check what a C++ file keeps to itself, against the rules in docs/contributing/Conventions.md
 * under "What a source file keeps to itself".
 *
 * The script reads every tracked .h, .cpp and .cxx file but the vendored ones, and reports:
 *
 *   type-in-source       a class, struct, union or enum defined at namespace scope in a source
 *                        file. A test file is exempt.
 *   mutable-variable     a variable at namespace scope that is not constant, including the
 *                        definition of a static data member. A pointer is constant only when
 *                        the pointer itself is const. A variable that a tracked header declares
 *                        extern is exempt from every rule, because an external standard gives it.
 *   mutable-static-local a static local in a function that is not constant, unless its line
 *                        ends with a comment starting "process-wide:".
 *   outside-anonymous    a variable or constant at namespace scope in a source file, outside the
 *                        anonymous namespace. A test file and a static data member's definition
 *                        are exempt.
 *   second-anonymous     a second anonymous namespace in one file.
 *   anonymous-late       an anonymous namespace that opens below a function definition.
 *   after-function       a variable, constant, type or type alias at namespace scope below a
 *                        function definition. A test case counts as a function definition.
 *   internal-include     an include of a header under an internal/ directory, from a file
 *                        outside the directory that holds that internal/. Such a header belongs
 *                        to its library's own sources and tests.
 *
 * A file is a test file when its path has a tests directory in it. The check reads code through
 * scripts/lib/lexer.ts, so nothing in a comment or a literal counts, and it skips preprocessor
 * lines. It does not parse C++. It follows braces, and sorts what comes before each brace or
 * semicolon at namespace scope by its first words. A variable initialised in parentheses is read
 * as one when the parentheses open on a literal, as in `std::atomic<int> next(1);`. One that opens
 * on a name, such as `Foo foo(bar);`, reads as a function declaration and is not reported.
 *
 * Usage: node scripts/filelocal.ts
 */

import { readFileSync } from 'node:fs';
import * as path from 'node:path';

import { options, report, runGate } from './lib/cli.ts';
import { tracked } from './lib/git.ts';
import { codeLines } from './lib/lexer.ts';
import { ROOT, isTestPath, isThirdParty } from './lib/tree.ts';

export type Rule = 'type-in-source' | 'mutable-variable' | 'mutable-static-local' | 'outside-anonymous' | 'second-anonymous' | 'anonymous-late'
    | 'after-function' | 'internal-include';

/** One report, and the line it is on. */
export interface Finding {
    line: number;
    rule: Rule;
    message: string;
}

/** What a file is, as the rules see it. */
export interface Kind {
    /** The file's path from the repository root. */
    path: string;
    /** A .cpp or .cxx file, rather than a header. */
    source: boolean;
    /** A file under a tests directory. */
    test: boolean;
    /** The names that the tree's headers declare extern. */
    externs: ReadonlySet<string>;
}

/** The kind of a repository path. */
export function kindOf(file: string, externs: ReadonlySet<string> = new Set()): Kind {
    return { path: file, source: /\.(cpp|cxx)$/.test(file), test: isTestPath(file), externs };
}

/**
 * Whether a file may include a header by its path from the root. A header under internal/ may
 * be included only from the directory that holds that internal/, or from below it.
 */
export function mayInclude(file: string, header: string): boolean {
    const at = header.lastIndexOf('/internal/');
    return at < 0 || file.startsWith(header.slice(0, at + 1));
}

/** Every name that an extern declaration in a header's text declares. */
export function externNames(text: string): string[] {
    const names: string[] = [];
    for (const statement of codeLines(text).join('\n').split(';')) {
        const declared = /(?:^|[{}\s])extern\s+(?!"")([^;{}]*)$/.exec(statement)?.[1];
        if (declared === undefined || declared.includes('(')) {
            continue;
        }
        // each declarator in a list such as `extern RtToken RI_A, RI_B;` is a name
        for (const declarator of declared.split(',')) {
            names.push(variableName(declarator));
        }
    }
    return names;
}

type Scope = 'namespace' | 'anonymous' | 'extern' | 'type' | 'body' | 'init';

/** The text after any template headers and attributes at its start. */
function stripPrefixes(text: string): string {
    let rest = text.trim();
    for (;;) {
        if (rest.startsWith('[[')) {
            const end = rest.indexOf(']]');
            if (end < 0) {
                return rest;
            }
            rest = rest.slice(end + 2).trim();
        } else if (/^template\s*</.test(rest)) {
            let depth = 0;
            let index = rest.indexOf('<');
            for (; index < rest.length; index++) {
                if (rest[index] === '<') {
                    depth++;
                } else if (rest[index] === '>') {
                    depth--;
                    if (depth === 0) {
                        break;
                    }
                }
            }
            rest = rest.slice(index + 1).trim();
        } else {
            // a macro invocation with no semicolon, such as BOOST_AUTO_TEST_SUITE(name), before
            // the declaration it precedes
            const macro = /^[A-Z][A-Z0-9_]*\s*\([^()]*\)\s*(?=\S)/.exec(rest);
            if (macro === null) {
                return rest;
            }
            rest = rest.slice(macro[0].length);
        }
    }
}

/** The name a type definition's head gives, such as Reader for `class Reader final : Base`. */
function typeName(head: string): string | null {
    const name = /^(?:typedef\s+)?(?:class|struct|union|enum)(?:\s+(?:class|struct))?\s*(?:alignas\s*\([^)]*\)\s*)?([\w:]*)/.exec(head)?.[1];
    return name === undefined ? null : (name === '' ? '(unnamed)' : name);
}

/** The name a variable declaration defines: the last identifier before its initialiser. */
function variableName(declarator: string): string {
    const names = declarator.replace(/\[[^\]]*\]/g, '').match(/[\w:]+/g);
    return names?.at(-1) ?? '';
}

/**
 * Whether a head has an = outside parentheses that is not part of an operator's name, as a
 * variable's initialiser does.
 */
function assigns(head: string): boolean {
    let depth = 0;
    for (let index = 0; index < head.length; index++) {
        const c = head[index];
        if (c === '(') {
            depth++;
        } else if (c === ')') {
            depth--;
        } else if (c === '=' && depth === 0) {
            const before = head.slice(0, index);
            const after = head[index + 1] ?? '';
            if (after !== '=' && !/[=!<>+\-*/%&|^]$/.test(before) && !/operator\s*$/.test(before)) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Whether a head ends inside a constructor's initialiser list, at a member initialised with
 * braces, such as `Item::Item() : id_{`. After its last parenthesis or brace it has a lone colon
 * or a comma, and it ends on a name.
 */
function memberInitialiser(head: string): boolean {
    const last = Math.max(head.lastIndexOf(')'), head.lastIndexOf('}'));
    const tail = head.slice(last + 1);
    return /\w\s*$/.test(tail) && (/(^|[^:]):([^:]|$)/.test(tail) || tail.includes(','));
}

/** Whether a variable declaration, without its initialiser, declares a constant. */
function isConstant(declarator: string): boolean {
    // a template's arguments, such as the const char* in std::array<const char*, 3>, say nothing
    // about whether the variable itself is constant
    let outer = declarator;
    for (let previous = ''; previous !== outer;) {
        previous = outer;
        outer = outer.replace(/<[^<>]*>/g, '');
    }
    if (/\bconstexpr\b/.test(outer)) {
        return true;
    }
    if (outer.includes('*')) {
        return /\*\s*const\b[^*]*$/.test(outer);
    }
    if (/&/.test(outer)) {
        return true;
    }
    return /\bconst\b/.test(outer);
}

/** Every finding in a file's text. */
export function findings(text: string, kind: Kind): Finding[] {
    const found: Finding[] = [];
    const add = (line: number, rule: Rule, message: string): void => {
        found.push({ line, rule, message });
    };
    const stack: Scope[] = [];
    let head = '';
    let headLine = 0;
    let parens = 0;
    let anonymous = 0;
    let firstFunction = 0;
    let afterTypedef = false;
    // the text since the last brace or semicolon inside a class or a function
    let inner = '';
    const atNamespaceScope = (): boolean => stack.every((s) => s === 'namespace' || s === 'anonymous' || s === 'extern');
    const inAnonymous = (): boolean => stack.includes('anonymous');

    /** Sort a statement that ended in a semicolon at namespace scope. */
    const statement = (raw: string, line: number): void => {
        const text = stripPrefixes(raw);
        if (text === '' || /^(using\s+namespace|namespace|extern|friend|static_assert|concept)\b/.test(text)) {
            return;
        }
        if (afterTypedef) {
            return;  // the name a typedef gives the type defined just before it
        }
        if (/^(class|struct|union|enum)(\s+class|\s+struct)?\s+[\w:]+(\s*:\s*[\w:\s]+)?$/.test(text)) {
            return;  // a forward declaration
        }
        if (/^(using|typedef)\b/.test(text)) {
            if (firstFunction > 0) {
                add(line, 'after-function', `the alias "${text.slice(0, 60)}" is declared below the function definition on line ${firstFunction}`);
            }
            return;
        }
        const equals = text.indexOf('=');
        const paren = text.indexOf('(');
        // parentheses that open on a literal initialise a variable, as in `std::atomic<int>
        // next(1)`
        const initialised = paren >= 0 && /^\(\s*([-+]?[\d.]|""|''|true\b|false\b|nullptr\b)/.test(text.slice(paren));
        if (paren >= 0 && (equals < 0 || paren < equals) && !initialised) {
            return;  // a function declaration, a macro invocation or a defaulted special member
        }
        const end = initialised && (equals < 0 || paren < equals) ? paren : equals;
        const declarator = (end >= 0 ? text.slice(0, end) : text).replace(/\{\}\s*$/, '').trim();
        const name = variableName(declarator);
        if (kind.externs.has(name)) {
            return;
        }
        const member = name.includes('::');
        if (!isConstant(declarator)) {
            add(line, 'mutable-variable', member
                ? `the static data member ${name} is mutable`
                : `${name} is a mutable variable at namespace scope`);
        }
        if (member) {
            return;
        }
        if (kind.source && !kind.test && !inAnonymous()) {
            add(line, 'outside-anonymous', `${name} is file-local and outside the anonymous namespace`);
        }
        if (firstFunction > 0) {
            add(line, 'after-function', `${name} is declared below the function definition on line ${firstFunction}`);
        }
    };

    /** Sort a head that ended in an opening brace at namespace scope, and say what it opens. */
    const opening = (raw: string, line: number): Scope => {
        const text = stripPrefixes(raw);
        if (/^namespace\s*$/.test(text)) {
            anonymous++;
            if (anonymous > 1) {
                add(line, 'second-anonymous', 'a second anonymous namespace; a file has one');
            }
            if (firstFunction > 0) {
                add(line, 'anonymous-late', `the anonymous namespace opens below the function definition on line ${firstFunction}`);
            }
            return 'anonymous';
        }
        if (/^(inline\s+)?namespace\b/.test(text)) {
            return 'namespace';
        }
        if (/^extern\s*""$/.test(text)) {
            return 'extern';
        }
        const type = typeName(text);
        if (type !== null && !text.includes('(')) {
            if (kind.source && !kind.test) {
                add(line, 'type-in-source', `${type} is defined in a source file; declare it in a header`);
            }
            if (firstFunction > 0) {
                add(line, 'after-function', `${type} is defined below the function definition on line ${firstFunction}`);
            }
            afterTypedef = /^typedef\b/.test(text);
            return 'type';
        }
        if (parens > 0 || assigns(text) || !text.includes('(') || memberInitialiser(text)) {
            return 'init';
        }
        if (firstFunction === 0) {
            firstFunction = line;
        }
        return 'body';
    };

    const lines = codeLines(text);
    const raw = text.split('\n');
    let continued = false;
    lines.forEach((code, index) => {
        const line = index + 1;
        const preprocessor = continued || code.trimStart().startsWith('#');
        continued = preprocessor && code.trimEnd().endsWith('\\');
        if (preprocessor) {
            // a quoted include names a header beside the file, so only the root-relative form
            // can reach another directory's internal/
            const header = /^\s*#\s*include\s*<([^>]+)>/.exec(raw[index] ?? '')?.[1];
            if (header !== undefined && !mayInclude(kind.path, header)) {
                add(line, 'internal-include', `${header} is internal to its library, and this file is outside it`);
            }
            return;
        }
        // a static declared in a function body, rather than in a class
        const local = /^\s*static\s+(?!_assert)([^=;({]*)/.exec(code)?.[1];
        if (local !== undefined && stack.length > 0 && stack[stack.length - 1] === 'body' && !isConstant(local)
            && !/\/\/\s*process-wide:/.test(raw[index] ?? '')) {
            add(line, 'mutable-static-local', `the static local ${variableName(local)} is mutable`);
        }
        for (const c of code) {
            if (!atNamespaceScope()) {
                if (c === '{') {
                    // a class nested in a class holds members, not locals
                    const nested = stripPrefixes(inner.replace(/^\s*((public|protected|private)\s*:\s*)+/, ''));
                    stack.push(typeName(nested) !== null && !inner.includes('(') ? 'type' : 'body');
                    inner = '';
                } else if (c === ';') {
                    inner = '';
                } else if (c !== '}') {
                    inner += c;
                }
                if (c === '}') {
                    inner = '';
                    const closed = stack.pop();
                    if (closed === 'init' && atNamespaceScope()) {
                        head += '{}';
                    }
                }
                continue;
            }
            if (head.trim() === '' && /\S/.test(c)) {
                headLine = line;
            }
            if (c === '(') {
                parens++;
                head += c;
            } else if (c === ')') {
                parens = Math.max(0, parens - 1);
                head += c;
            } else if (c === '{') {
                const scope = opening(head, headLine);
                stack.push(scope);
                if (scope !== 'init') {
                    head = '';
                }
            } else if (c === '}') {
                stack.pop();
                head = '';
            } else if (c === ';' && parens === 0) {
                statement(head, headLine);
                head = '';
                afterTypedef = false;
            } else {
                head += c;
            }
        }
        head += ' ';
    });
    return found;
}

/** The tracked C++ files the check reads, relative to the root. */
export function files(): string[] {
    return tracked('*.h', '*.cpp', '*.cxx').filter((f) => !isThirdParty(f));
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    options('usage: node scripts/filelocal.ts', {});
    const checked = files();
    const externs = new Set(checked.filter((f) => f.endsWith('.h')).flatMap((f) => externNames(readFileSync(path.join(ROOT, f), 'utf8'))));
    const found = checked.flatMap((file) => findings(readFileSync(path.join(ROOT, file), 'utf8'), kindOf(file, externs))
        .map(({ line, rule, message }) => ({ path: file, line, rule, message })));
    return report(found, 'breaks "What a source file keeps to itself" in docs/contributing/Conventions.md.');
}

runGate(import.meta.url, main);
