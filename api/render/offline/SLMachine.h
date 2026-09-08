/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "SLProgram.h"
#include "SLRenderer.h"
#include "SLValue.h"

namespace v3d::render::offline {

/**
 * Runs a program over a batch of shading points.
 *
 * **The execution mask is a stack.** A condition every point agrees about is a jump; one they
 * disagree about runs both arms, each under the lanes that took it, and an arm no lane took
 * is skipped - which is the optimisation that makes the common case free. A loop iterates
 * while any lane is live, and `break`, `continue` and `return` clear lanes rather than
 * jumping out, because the lanes beside them have not finished.
 *
 * **talyn's batch is one point.** No special case and no second path: the same program, the
 * same instructions, a mask one bit wide. That is the whole reason the model is a batch.
 *
 * **A run allocates once.** prepare() sizes the register file for a program and a batch; a
 * renderer then writes its inputs into the file, calls run(), and reads the results out - a
 * thousand grids over one program touch the same memory.
 **/
class SLMachine final {
 public:
    /**
     * Size the register file for this program and batch, and write its constants in. A batch
     * of zero is read as one.
     **/
    void prepare(const SLProgram & program, unsigned int batch);

    /**
     * Run it. False when something went wrong, which error() names.
     **/
    bool run(const SLProgram & program);

    /**
     * A register, by the index the program gave it. The first `SLProgram::symbols` of them
     * are the shader's symbols, which is how a renderer writes P and reads Ci.
     **/
    SLValue & value(int reg);
    const SLValue & value(int reg) const;

    unsigned int batch() const;

    /**
     * What the machine asks a renderer for. Null until one is attached, which leaves a named
     * coordinate space unchanged and reported.
     **/
    void renderer(SLRenderer* renderer);

    const std::string & error() const;

    /**
     * What a run said about the things it could not do - a space no renderer knew, a
     * built-in with no body yet. One line per distinct message, because a scene that rendered
     * wrong and a scene that was not understood look identical from outside, and a 640 by 480
     * render must not print a million lines to say so.
     **/
    const std::vector<std::string> & reports() const;

 private:
    /**
     * A loop in progress: the lanes still going round it, and how deep the mask stack was
     * when it opened, so that a break can clear the lanes out of every mask inside it.
     **/
    class Loop final {
     public:
        std::vector<char> lanes;
        std::size_t depth = 0;
        int exit = 0;
    };

    bool live(unsigned int point) const;
    bool anyLive() const;
    /** Whether an instruction writing this value should write this point of it. **/
    bool writable(const SLValue & target, unsigned int point) const;
    void report(const std::string & message);

    void move(const SLInstruction & instruction);  // NOLINT(build/include_what_you_use) - the name, not std::move
    void component(const SLInstruction & instruction);
    void arithmetic(const SLInstruction & instruction);
    void compare(const SLInstruction & instruction);
    void product(const SLInstruction & instruction);
    void unary(const SLInstruction & instruction);
    void transform(const SLInstruction & instruction);  // NOLINT(build/include_what_you_use) - the name, not std::transform
    /** Push the lanes of the condition that are, or are not, non-zero. **/
    void mask(const SLInstruction & instruction, bool wanted);
    /** Narrow a loop to the lanes its condition still holds. **/
    void narrow(const SLInstruction & instruction);
    /** Take the live lanes out of everything: they are done with this shader. **/
    void finish();
    void leave(bool loop);

    std::vector<SLValue> file_;
    std::vector<std::vector<char> > masks_;
    std::vector<Loop> loops_;
    std::vector<std::string> reports_;
    SLRenderer* renderer_ = nullptr;
    unsigned int batch_ = 1;
    std::string error_;
};

};  // namespace v3d::render::offline
