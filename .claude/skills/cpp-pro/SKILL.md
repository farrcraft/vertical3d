---
name: cpp-pro
description: Writes, optimizes, and debugs C++ applications using modern C++20/23 features, template metaprogramming, and high-performance systems techniques. Use when building or refactoring C++ code requiring concepts, ranges, coroutines, SIMD optimization, or careful memory management — or when addressing performance bottlenecks, concurrency issues, and build system configuration with CMake.
license: MIT
metadata:
  author: https://github.com/Jeffallan
  version: "1.1.0"
  domain: language
  triggers: C++, C++20, C++23, modern C++, template metaprogramming, systems programming, performance optimization, SIMD, memory management, CMake
  role: specialist
  scope: implementation
  output-format: code
  related-skills: systematic-debugging, architecture-decision-records
---

# C++ Pro

Senior C++ developer with deep expertise in modern C++20/23, systems programming, high-performance computing, and zero-overhead abstractions.

## Core Workflow

1. **Analyze architecture** — Review build system, compiler flags, performance requirements
2. **Design with concepts** — Create type-safe interfaces using C++20 concepts
3. **Implement zero-cost** — Apply RAII, constexpr, and zero-overhead abstractions
4. **Verify quality** — Run whatever static analysis the project actually has, and fix what
   it reports. Here that is cpplint, the compiler at `/W4 /WX`, MSVC `/analyze` and
   clang-tidy; there is no sanitizer build
5. **Benchmark** — Profile with real workloads; if performance targets are not met, apply targeted optimizations (SIMD, cache layout, move semantics) and re-measure

## Reference Guide

Load detailed guidance based on context:

| Topic | Reference | Load When |
|-------|-----------|-----------|
| Modern C++ Features | `references/modern-cpp.md` | C++20/23 features, concepts, ranges, coroutines |
| Template Metaprogramming | `references/templates.md` | Variadic templates, SFINAE, type traits, CRTP |
| Memory & Performance | `references/memory-performance.md` | Allocators, SIMD, cache optimization, move semantics |
| Concurrency | `references/concurrency.md` | Atomics, lock-free structures, thread pools, coroutines |
| Build & Tooling | `references/build-tooling.md` | CMake, sanitizers, static analysis, testing |

## In this repository

The guidance below is generic modern C++. Where it disagrees with Vertical3D's own
conventions, **the repository wins** — `docs/contributing/Conventions.md` is the authority
and consistency with the surrounding code matters more than idiom.

What differs here:

- **`boost::shared_ptr` and `boost::make_shared`, not the `std` equivalents.** This is
  consistent across the whole tree. Do not "modernise" it in passing.
- **Doc comments are `/** **/` blocks**, not `///`.
- Headers are `.h`; implementations are `.cpp` **or** `.cxx`, mixed even within a directory.
  Match the immediate neighbours.
- The root sets `CMAKE_CXX_STANDARD 23` (MSVC `/std:c++latest`) and `/permissive-`, so modern
  language features are available. Do not add `/std:c++latest` as a flag. Most of this code is
  conservative and much of it predates C++11. Reach for concepts, ranges and coroutines when
  they earn their place, not to demonstrate them.
- **Warnings are errors.** Every target builds at `/W4` with `/WX` (`V3D_WARNINGS_AS_ERRORS`,
  on by default) and `/w14062`. The tree is clean, so every warning is a new one. Fix it rather
  than suppressing it.
- **Four checks, all clean.** cpplint (`--linelength=180`, no `--filter`), the compiler, MSVC
  `/analyze` (`-DV3D_ANALYZE=ON`) and clang-tidy (`-DV3D_CLANG_TIDY=ON`, checks in
  `.clang-tidy`). Every finding from any of them is new. `docs/contributing/Linting.md` says how
  to run each. There is no sanitizer build.
- **What the tests can prove.** Each api library and most apps have a Boost.Test suite behind
  ctest, so a testable CPU change can be proved. The `render_device` suite draws on a real
  Vulkan device, offscreen, and asserts validation silence and the pixels, a few of them
  against committed reference images. It runs on lavapipe in CI. A window, audible sound and
  anything blended or filtered beyond what a case checks cannot be proved by a test.
  `docs/contributing/Testing.md` covers both. Do not claim verification you did not perform.

## Constraints

### MUST DO
- Follow C++ Core Guidelines
- Use concepts for template constraints
- Apply RAII universally
- Use `auto` with type deduction
- Prefer smart pointers over raw owning pointers — `boost::shared_ptr` in this repository,
  `std::unique_ptr`/`std::shared_ptr` in general C++
- Leave the compiler's diagnostics alone: fix what they report rather than silencing it
- Write const-correct code

### MUST NOT DO
- Use raw `new`/`delete` (prefer smart pointers)
- Ignore compiler warnings
- Use C-style casts (use static_cast, etc.)
- Mix exception and error code patterns inconsistently
- Write non-const-correct code
- Use `using namespace std` in headers
- Ignore undefined behavior
- Skip move semantics for expensive types

## Key Patterns

### Concept Definition (C++20)
```cpp
// Define a reusable, self-documenting constraint
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<Numeric T>
T clamp(T value, T lo, T hi) {
    return std::clamp(value, lo, hi);
}
```

### RAII Resource Wrapper
```cpp
// Wraps a raw handle; no manual cleanup needed at call sites
class FileHandle {
public:
    explicit FileHandle(const char* path)
        : handle_(std::fopen(path, "r")) {
        if (!handle_) throw std::runtime_error("Cannot open file");
    }
    ~FileHandle() { if (handle_) std::fclose(handle_); }

    // Non-copyable, movable
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle(FileHandle&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr)) {}

    std::FILE* get() const noexcept { return handle_; }
private:
    std::FILE* handle_;
};
```

### Smart Pointer Ownership
```cpp
// Prefer make_unique / make_shared; avoid raw new/delete
auto buffer = std::make_unique<std::array<std::byte, 4096>>();

// Shared ownership only when genuinely needed
auto config = std::make_shared<Config>(parseArgs(argc, argv));
```

## Output Templates

When implementing C++ features, provide:
1. Header file with interfaces and templates
2. Implementation file (when needed)
3. CMakeLists.txt updates (if applicable)
4. Test file demonstrating usage
5. Brief explanation of design decisions and performance characteristics

[Documentation](https://jeffallan.github.io/claude-skills/skills/language/cpp-pro/)
