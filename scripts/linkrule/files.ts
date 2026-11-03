/**
 * The files the link rule reads, and the paths it reports them by.
 */

import { existsSync, readFileSync, statSync } from 'node:fs';
import * as path from 'node:path';

export const SOURCE_SUFFIXES = ['.h', '.hpp', '.inl', '.c', '.cpp', '.cxx'];
export const HEADER_SUFFIXES = ['.h', '.hpp', '.inl'];

/** A path relative to a root, with forward slashes. */
export function rel(root: string, p: string): string {
    return path.relative(root, p).split(path.sep).join('/');
}

export function read(p: string): string {
    return readFileSync(p, 'utf-8').replace(/^﻿/, '');
}

export function isFile(p: string): boolean {
    return existsSync(p) && statSync(p).isFile();
}

export function endsWithAny(name: string, suffixes: string[]): boolean {
    return suffixes.some((s) => name.endsWith(s));
}
