/**
 * What a C++ file includes and which v3d namespaces it names, read from its text.
 */

import { blankComments, codeLines } from '../lib/lexer.ts';

// The header paths each package target provides, matched against the text of an include. The
// specific Boost components come before Boost::headers, which takes the rest of boost/.
const PACKAGE_HEADERS: Array<[string, RegExp]> = [
    ['Boost::filesystem', /^boost\/filesystem/],
    ['Boost::json', /^boost\/json/],
    ['Boost::program_options', /^boost\/program_options/],
    ['Boost::unit_test_framework', /^boost\/test\//],
    ['Boost::headers', /^boost\//],
    ['glm::glm', /^glm\//],
    ['EnTT::EnTT', /^entt\//],
    ['SDL3_mixer::SDL3_mixer', /^SDL3_mixer\//],
    ['SDL3::SDL3', /^SDL3\//],
    ['Freetype::Freetype', /^(ft2build\.h|freetype\/)/],
    ['PNG::PNG', /^png\.h$/],
    ['JPEG::JPEG', /^(jpeglib|jerror|jconfig|jmorecfg)\.h$/],
    ['Vulkan::Vulkan', /^vulkan\//],
    ['GPUOpen::VulkanMemoryAllocator', /^vk_mem_alloc/],
    ['spdlog::spdlog', /^(spdlog|fmt)\//],
    ['libnoise', /^(noise\.h|interp\.h|mathconsts\.h|noise\/)/],
];

/**
 * Remove comments and #if 0 blocks, and reduce string and character literals to their quotes
 * unless asked to keep them.
 */
export function stripCpp(text: string, keepStrings: boolean): string {
    const lines = keepStrings ? blankComments(text).split('\n') : codeLines(text);
    const kept: string[] = [];
    let depth = 0;
    for (const line of lines) {
        const s = line.trim();
        if (depth > 0) {
            if (/^#\s*if/.test(s)) {
                depth += 1;
            } else if (/^#\s*endif/.test(s)) {
                depth -= 1;
            } else if (depth === 1 && /^#\s*(else|elif)/.test(s)) {
                depth = 0;
            }
            kept.push('');
        } else if (/^#\s*if\s+0\b/.test(s)) {
            depth = 1;
            kept.push('');
        } else {
            kept.push(line);
        }
    }
    return kept.join('\n');
}

const INCLUDE = /^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]/gm;

export const DECLARATION = /\bnamespace\s+v3d((?:::\w+)+)\s*\{/g;
export const FORWARD_ONLY = /\s*(?:(?:class|struct)\s+\w+\s*;\s*)*\}/y;

// A namespace declaration such as "namespace v3d::grid {" names no use, and a using-directive
// such as "using namespace v3d::grid;" does.
const QUALIFIED = /(?<![\w:])(using\s+)?(namespace\s+)?v3d((?:\s*::\s*\w+)+)/g;

/** The text of each include in a C++ file, in order. */
export function includesIn(text: string): string[] {
    return [...stripCpp(text, true).matchAll(INCLUDE)].map((m) => m[1]);
}

/**
 * The names in a v3d::<lib>:: namespace that a C++ file uses, each with the libraries that declare
 * the longest namespace of it found in namespaces.
 */
export function usesIn(text: string, namespaces: Map<string, Set<string>>): Array<[string[], string]> {
    const uses: Array<[string[], string]> = [];
    for (const m of stripCpp(text, false).matchAll(QUALIFIED)) {
        if (m[2] && !m[1]) {
            continue;
        }
        const parts = m[3].split('::').slice(1).map((p) => p.trim());
        for (let k = parts.length; k > 0; k -= 1) {
            const libs = namespaces.get(parts.slice(0, k).join('::'));
            if (libs !== undefined) {
                uses.push([[...libs].sort(), 'v3d::' + parts.join('::')]);
                break;
            }
        }
    }
    return uses;
}

export function packageOf(include: string): string | null {
    for (const [target, pattern] of PACKAGE_HEADERS) {
        if (pattern.test(include)) {
            return target;
        }
    }
    return null;
}
