/**
 * Delete the cache entries a save has replaced, so that the repository's cache stays within its
 * size.
 *
 * A workflow saves its vcpkg archives under a key that ends in the run's id, so each run that
 * builds something stores a whole new copy. A restore takes the newest entry its restore keys
 * match. Once a run has saved, no older entry on the same ref is read again, whether its restore
 * keys match it or no longer do. After a save, this deletes
 * every entry on the run's ref whose key starts with a prefix, but those this run saved. Entries
 * of other refs are left alone, so a pull request never deletes what main saved.
 *
 * The prefix is the runner's system, and the compiler's toolset where the key names it. An entry
 * of another toolset is not under it, so while runners on two images build, each keeps its own.
 *
 * Pruning never fails a run. A deletion GitHub refuses is reported as a warning, and the cache
 * evicts what is left once it is full or unread for a week.
 *
 * Usage: node scripts/prunecache.ts --prefix <prefix> --saved <key>
 *
 *   --prefix  the prefix whose older entries the run's restore will not read again.
 *   --saved   the key this run saved under, up to and including the run's id. A job that adds
 *             to it, such as a build type, keeps its entry too.
 *
 * It reads GITHUB_TOKEN, GITHUB_REPOSITORY and GITHUB_REF, as a workflow's step sets them. The
 * token needs the actions: write permission.
 */

import process from 'node:process';
import { parseArgs } from 'node:util';

import { isEntryPoint } from './lib/entry.ts';

const USAGE = 'usage: node scripts/prunecache.ts --prefix <prefix> --saved <key>';

/** GitHub's REST API, and the most entries one page of a listing holds. */
const API = 'https://api.github.com';
const PAGE = 100;

/** One cache entry, as GitHub lists it. */
export interface Entry {
    id: number;
    key: string;
}

/**
 * The entries to delete: each whose key starts with the prefix and was not saved by the run. A key
 * the run saved is the saved key, alone or followed by a dash and what a job adds. Nothing is
 * deleted unless the entries hold one the run saved, so a save that did not land leaves the older
 * entries for the next restore.
 */
export function replaced(entries: readonly Entry[], prefix: string, saved: string): Entry[] {
    const own = (key: string): boolean => key === saved || key.startsWith(`${saved}-`);
    if (!entries.some(({ key }) => own(key))) {
        return [];
    }
    return entries.filter(({ key }) => key.startsWith(prefix) && !own(key));
}

/** The headers of a request to GitHub's API with a token. */
function headers(token: string): Record<string, string> {
    return { Accept: 'application/vnd.github+json', Authorization: `Bearer ${token}`, 'X-GitHub-Api-Version': '2022-11-28' };
}

/** Whether a value is a page of GitHub's listing of cache entries. */
export function isListing(value: unknown): value is { actions_caches: Entry[] } {
    if (typeof value !== 'object' || value === null || !('actions_caches' in value) || !Array.isArray(value.actions_caches)) {
        return false;
    }
    return value.actions_caches.every(
        (entry: unknown) =>
            typeof entry === 'object' &&
            entry !== null &&
            'id' in entry &&
            typeof entry.id === 'number' &&
            'key' in entry &&
            typeof entry.key === 'string',
    );
}

/** Every entry on a ref whose key starts with a prefix, from a page of the listing on. */
async function listed(repository: string, ref: string, prefix: string, token: string, page = 1): Promise<Entry[]> {
    const query = new URLSearchParams({ key: prefix, ref, per_page: String(PAGE), page: String(page) });
    const response = await fetch(`${API}/repos/${repository}/actions/caches?${query.toString()}`, { headers: headers(token) });
    if (!response.ok) {
        throw new Error(`listing the cache gave ${response.status}`);
    }
    const body: unknown = await response.json();
    if (!isListing(body)) {
        throw new Error('the cache listing was not one of entries');
    }
    if (body.actions_caches.length < PAGE) {
        return body.actions_caches;
    }
    return [...body.actions_caches, ...(await listed(repository, ref, prefix, token, page + 1))];
}

/** Delete one entry, and say whether GitHub did. */
async function deleted(repository: string, entry: Entry, token: string): Promise<void> {
    const response = await fetch(`${API}/repos/${repository}/actions/caches/${entry.id}`, { method: 'DELETE', headers: headers(token) });
    console.log(response.ok ? `deleted ${entry.key}` : `::warning::could not delete ${entry.key}: ${response.status}`);
}

async function main(): Promise<number> {
    const { values } = parseArgs({ options: { prefix: { type: 'string' }, saved: { type: 'string' } } });
    const token = process.env['GITHUB_TOKEN'];
    const repository = process.env['GITHUB_REPOSITORY'];
    const ref = process.env['GITHUB_REF'];
    if (!values.prefix || !values.saved || !token || !repository || !ref) {
        console.error(USAGE);
        console.error('GITHUB_TOKEN, GITHUB_REPOSITORY and GITHUB_REF must be set');
        return 2;
    }
    try {
        const stale = replaced(await listed(repository, ref, values.prefix, token), values.prefix, values.saved);
        await Promise.all(stale.map((entry) => deleted(repository, entry, token)));
        console.log(`${stale.length} replaced entries under ${values.prefix}`);
    } catch (error) {
        console.log(`::warning::the cache was not pruned: ${error instanceof Error ? error.message : String(error)}`);
    }
    return 0;
}

if (isEntryPoint(import.meta.url)) {
    process.exitCode = await main();
}
