/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Program.h"
#include "Renderer.h"
#include "Value.h"

namespace v3d::render::offline::sl::runtime {

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
class Machine final {
 public:
    /**
     * Size the register file for this program and batch, and write its constants in. A batch
     * of zero is read as one.
     **/
    void prepare(const Program & program, unsigned int batch);

    /**
     * Run it. False when something went wrong, which error() names.
     *
     * Starts after the program's prologue, which is the declared parameter defaults:
     * running those again per grid would overwrite whatever a scene bound.
     **/
    bool run(const Program & program);

    /**
     * Run only the prologue, which leaves each parameter register holding the default the
     * shader declared. What `Shader` reads its defaults out of, once.
     **/
    bool initialise(const Program & program);

    /**
     * A register, by the index the program gave it. The first `Program::symbols` of them
     * are the shader's symbols, which is how a renderer writes P and reads Ci.
     **/
    Value & value(int reg);
    const Value & value(int reg) const;

    unsigned int batch() const;

    /**
     * What the machine asks a renderer for. Null until one is attached, which leaves a named
     * coordinate space unchanged and reported.
     **/
    void renderer(Renderer* renderer);

    const std::string & error() const;

    /**
     * What a run said about the things it could not do - a space no renderer knew, a
     * built-in with no body yet. One line per distinct message, because a scene that rendered
     * wrong and a scene that was not understood look identical from outside, and a 640 by 480
     * render must not print a million lines to say so.
     **/
    const std::vector<std::string> & reports() const;

    /**
     * What a `printf` in the shader wrote, in the order it wrote it, cleared at the start
     * of every run. A renderer drains it into its log and a case reads it.
     *
     * Not reports(), which says one thing once: a person who wrote a printf is asking to be
     * told every time, and a line per shading point is what they asked for.
     **/
    const std::vector<std::string> & printed() const;

    /**
     * Which points a light shader's run lit, for the renderer to hand back to the surface
     * shader's illuminance loop.
     *
     * Every point until an `illuminate` or a `solar` narrows it, which is what makes a
     * light shader with neither - `ambientlight` - light the whole batch.
     **/
    const std::vector<char> & lit() const;

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

    /**
     * An inlined function body in progress: how deep the mask and loop stacks were when it
     * opened, so that a return clears the lanes out of everything inside it and out of
     * nothing beyond it.
     **/
    class Frame final {
     public:
        std::size_t masks = 0;
        std::size_t loops = 0;
    };

    /**
     * An illuminance loop in progress: which light the body runs for next, the lanes the
     * loop opened with, and the registers the construct named.
     **/
    class Illumination final {
     public:
        unsigned int light = 0;
        std::vector<char> base;
        int direction = -1;
        int colour = -1;
        std::vector<int> arguments;
    };

    /** One pass over a range of the instructions, which is what both entry points are. **/
    bool execute(const Program & program, std::size_t from, std::size_t until);

    bool live(unsigned int point) const;
    bool anyLive() const;
    /** Whether an instruction writing this value should write this point of it. **/
    bool writable(const Value & target, unsigned int point) const;
    void report(const std::string & message);

    void move(const Instruction & instruction);  // NOLINT(build/include_what_you_use) - the name, not std::move
    void component(const Instruction & instruction);
    void arithmetic(const Instruction & instruction);
    void compare(const Instruction & instruction);
    void product(const Instruction & instruction);
    void unary(const Instruction & instruction);
    void transform(const Instruction & instruction);  // NOLINT(build/include_what_you_use) - the name, not std::transform
    /**
     * A standard library call. Defined in Library.cxx, which is most of the language by
     * volume and none of it by mechanism: every body there is arithmetic over the value
     * model, and the four that are not ask the renderer.
     **/
    void builtin(const Instruction & instruction);
    /**
     * The sum of what every ambient light adds to the batch. Its own body rather than one
     * of Library.cxx's, because it is the one built-in that runs the lights itself: an
     * ambient light has no direction, so an illuminance loop cannot reach it.
     **/
    void ambient(Value* target);
    /**
     * transmission and trace, which are the two the renderer answers about a line between
     * two points. A renderer that cannot lets all the light through and traces nothing,
     * and says which.
     **/
    void shadowed(bool ray, const Value & from, const Value & to, Value* target);
    /**
     * The matrix into a named coordinate space, the identity and a report when no renderer
     * knows it - a scene that named a space nothing knows renders in the wrong place rather
     * than not at all, and says so.
     **/
    glm::mat4x4 space(const std::string & name);
    /** Push the lanes of the condition that are, or are not, non-zero. **/
    void mask(const Instruction & instruction, bool wanted);
    /** Narrow a loop to the lanes its condition still holds. **/
    void narrow(const Instruction & instruction);
    /**
     * Narrow the batch to the points one light reaches, whichever end of the message
     * passing asked. False when there is no such point, and the body is left over.
     **/
    bool admit(const Instruction & instruction);
    /**
     * Set L and Cl from the next light that reaches any point of the batch, and push the
     * lanes it reaches. False when the lights have run out.
     **/
    bool nextLight();
    /**
     * The lanes a light shader's illuminate or solar admits, having written L for each of
     * them. False when it admits none, which is a light aimed away from this batch.
     **/
    bool illuminate(const Instruction & instruction, bool solar);
    /**
     * Whether a direction lies inside the cone an axis and a half angle name. A cone that
     * named neither takes everything.
     **/
    bool inside(const glm::vec3 & direction, const std::vector<int> & cone,
        std::size_t first, unsigned int point) const;
    /**
     * Take the live lanes out of the innermost inlined body, or out of everything when
     * there is none.
     **/
    void finish();
    void leave(bool loop);

    std::vector<Value> file_;
    std::vector<std::vector<char> > masks_;
    std::vector<Loop> loops_;
    std::vector<Frame> frames_;
    std::vector<Illumination> illuminations_;
    /**
     * Where an ambient light's answer lands before it is added in. Members rather than
     * locals so that a grid summing the same two lights a thousand times allocates once.
     **/
    Value direction_;
    Value colour_;
    /** The register P is, for the built-in that asks the lights about the batch. **/
    int point_ = -1;
    std::vector<char> lit_;
    std::vector<std::string> reports_;
    std::vector<std::string> printed_;
    Renderer* renderer_ = nullptr;
    unsigned int batch_ = 1;
    std::string error_;
};

};  // namespace v3d::render::offline::sl::runtime
