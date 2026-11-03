/**
 * The mdBook preprocessor that points each link the site cannot serve at the file on GitHub.
 *
 * A document in docs/ links files anywhere in the repository, such as a header, a CMakeLists or a
 * record under a completed/ directory. The site holds only the pages SUMMARY.md lists and the
 * other files in docs/. A link to anything else is rewritten to the file or directory on GitHub,
 * with its anchor kept. A link to a directory whose README.md is a page is pointed at that page.
 * A relative link to a path that does not exist is an error, and the build stops.
 *
 * mdBook runs this with "supports <renderer>" first, then with [context, book] as JSON on
 * standard input. The book with its chapters rewritten goes to standard output. Links inside code
 * spans and fenced blocks are left as they are.
 */

import { existsSync, readFileSync, statSync } from 'node:fs';
import * as path from 'node:path';
import process from 'node:process';

import { shouldRun } from '../lib/entry.ts';
import { ROOT } from '../lib/tree.ts';

/** What the rewrite needs to know about the repository. Paths are relative to its root. */
export interface Site {
    /** The pages the book holds, relative to docs/. */
    pages: ReadonlySet<string>;
    /** The repository's URL on GitHub, and the branch its links name. */
    repository: string;
    branch: string;
    /** Whether a path exists, and whether it is a directory. */
    kind: (file: string) => 'file' | 'directory' | null;
}

/** A link that names nothing, with the page and the target. */
export interface Broken {
    page: string;
    target: string;
}

/** The target a link should have on the site, or null when it names nothing. */
export function siteTarget(page: string, target: string, site: Site): string | null {
    if (/^[a-z][a-z0-9+.-]*:/i.test(target) || target.startsWith('#') || target === '') {
        return target;
    }
    const hash = target.indexOf('#');
    const file = hash < 0 ? target : target.slice(0, hash);
    const anchor = hash < 0 ? '' : target.slice(hash);
    const repoPath = path.posix.normalize(path.posix.join('docs', path.posix.dirname(page), file)).replace(/\/$/, '');
    const kind = site.kind(repoPath);
    if (kind === null) {
        return null;
    }
    const inDocs = repoPath.startsWith('docs/');
    const docsPath = inDocs ? repoPath.slice('docs/'.length) : '';
    if (inDocs && kind === 'file' && (site.pages.has(docsPath) || !docsPath.endsWith('.md'))) {
        return target;
    }
    if (inDocs && kind === 'directory' && site.pages.has(`${docsPath}/README.md`)) {
        return `${file.replace(/\/?$/, '/')}README.md${anchor}`;
    }
    const view = kind === 'directory' ? 'tree' : 'blob';
    return `${site.repository}/${view}/${site.branch}/${repoPath}${anchor}`;
}

/** A page's Markdown, each link given its site target. A link that names nothing goes in broken. */
export function rewrite(page: string, content: string, site: Site, broken: Broken[]): string {
    let fence: string | null = null;
    return content.split('\n').map((line) => {
        const opener = /^\s*(`{3,}|~{3,})/.exec(line);
        if (opener !== null) {
            const mark = opener[1] ?? '';
            if (fence === null) {
                fence = mark;
            } else if (mark.startsWith(fence)) {
                fence = null;
            }
            return line;
        }
        if (fence !== null) {
            return line;
        }
        // the odd parts of the split are code spans, which hold no links
        return line.split(/(`[^`]*`)/).map((part, index) => {
            if (index % 2 === 1) {
                return part;
            }
            return part.replace(/\]\(([^)\s]+)\)/g, (whole, target: string) => {
                const resolved = siteTarget(page, target, site);
                if (resolved === null) {
                    broken.push({ page, target });
                    return whole;
                }
                return `](${resolved})`;
            });
        }).join('');
    }).join('\n');
}

/** A chapter as mdBook's JSON gives it, with the fields this reads and writes. */
interface Chapter {
    content: string;
    source_path: string | null;
    sub_items: unknown[];
}

/** Each chapter in a list of book items, depth first. */
function chapters(items: readonly unknown[]): Chapter[] {
    const found: Chapter[] = [];
    for (const item of items) {
        if (typeof item === 'object' && item !== null && 'Chapter' in item) {
            const chapter = (item as { Chapter: Chapter }).Chapter;
            found.push(chapter, ...chapters(chapter.sub_items));
        }
    }
    return found;
}

/** What a path relative to the repository root is, read from the disk. */
function diskKind(file: string): 'file' | 'directory' | null {
    const full = path.join(ROOT, file);
    if (!existsSync(full)) {
        return null;
    }
    return statSync(full).isDirectory() ? 'directory' : 'file';
}

function main(): void {
    if (process.argv[2] === 'supports') {
        process.exitCode = 0;
        return;
    }
    const [context, book] = JSON.parse(readFileSync(0, 'utf8')) as [
        { config: { preprocessor: { outside: { repository: string; branch: string } } } },
        { items: unknown[] },
    ];
    const settings = context.config.preprocessor.outside;
    const all = chapters(book.items);
    const site: Site = {
        pages: new Set(all.flatMap((chapter) => chapter.source_path === null ? [] : [chapter.source_path])),
        repository: settings.repository,
        branch: settings.branch,
        kind: diskKind,
    };
    const broken: Broken[] = [];
    for (const chapter of all) {
        if (chapter.source_path !== null) {
            chapter.content = rewrite(chapter.source_path, chapter.content, site, broken);
        }
    }
    for (const link of broken) {
        process.stderr.write(`docs/${link.page}: ${link.target} names nothing\n`);
    }
    if (broken.length > 0) {
        process.exitCode = 1;
        return;
    }
    process.stdout.write(JSON.stringify(book));
}

if (shouldRun(import.meta.url)) {
    main();
}
