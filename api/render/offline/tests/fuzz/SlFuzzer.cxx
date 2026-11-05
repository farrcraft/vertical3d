/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

// Passes each input to the shading language parser as a source file, and compiles every
// shader it parses. Both stop at the first error, so the target checks only that no input
// makes the lexer, the parser or the compiler misbehave.

#include <api/render/offline/sl/compiler/Compiler.h>
#include <api/render/offline/sl/compiler/Parser.h>
#include <api/render/offline/sl/syntax/Shader.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    std::istringstream stream(std::string(reinterpret_cast<const char*>(data), size));
    v3d::render::offline::sl::compiler::Parser parser(stream);
    const std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parser.parse();
    for (const v3d::render::offline::sl::syntax::ShaderPtr & shader : shaders) {
        v3d::render::offline::sl::compiler::Compiler compiler(shader);
        compiler.compile();
    }
    return 0;
}
