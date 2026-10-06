/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Instruction.h"
#include "Program.h"
#include "Renderer.h"
#include "Value.h"

namespace v3d::render::offline::sl::runtime {

/**
 * Runs a program over a batch of shading points.
 *
 * **The execution mask is a stack.** A condition with the same value at every point is a
 * jump. A condition whose value differs runs both arms, each under the lanes that took it,
 * and an arm no lane took is skipped. A loop iterates while any lane is live, and `break`,
 * `continue` and `return` clear lanes rather than jumping out, because the lanes beside
 * them have not finished.
 *
 * **A traced hit's batch is one point.** It runs the same program and the same instructions
 * with a mask one bit wide, with no special case and no second path.
 *
 * **A run allocates once.** prepare() sizes the register file for a program and a batch; a
 * renderer then writes its inputs into the file, calls run(), and reads the results out - a
 * thousand grids over one program touch the same memory.
 **/
class Machine final {
 public:
    /**
     * Size the register file for this program and batch, write its constants in, and hold the
     * program for run() and initialise(), so a register file cannot be run with a program it
     * was not sized for. The program has to outlive the machine's use of it. A batch of zero is
     * read as one.
     **/
    void prepare(const Program & program, unsigned int batch);

    /**
     * Run the program prepare() was given. False when something went wrong, which error()
     * names - and when nothing was prepared.
     *
     * Starts after the program's prologue, which is the declared parameter defaults:
     * running those again per grid would overwrite whatever a scene bound.
     **/
    bool run();

    /**
     * Run only the prologue, which leaves each parameter register holding the default the
     * shader declared. `Instance::write()` calls it before writing a scene's values.
     **/
    bool initialise();

    /**
     * A register, by the index the program gave it. The first `Program::symbols` of them
     * are the shader's symbols, which is how a renderer writes P and reads Ci.
     **/
    Value & value(int reg);
    const Value & value(int reg) const;

    unsigned int batch() const;

    /**
     * The renderer that supplies what the machine does not hold. Null until one is attached,
     * which leaves a named coordinate space unchanged and reported.
     **/
    void renderer(Renderer* renderer);

    const std::string & error() const;

    /**
     * Reports of what a run could not do, such as a space no renderer knew or a built-in with
     * no body. One line per distinct message, so a 640 by 480 render does not print a million
     * copies of one message.
     **/
    const std::vector<std::string> & reports() const;

    /**
     * Where a report also goes, as a warning, the first time it is made. Without one, reports
     * are only kept for reports() to read.
     **/
    void logger(const boost::shared_ptr<v3d::log::Logger> & logger);

    /**
     * Keeps a message for reports(), and logs it, once however often it is made.
     **/
    void report(const std::string & message);

    /**
     * What a `printf` in the shader wrote, in the order it wrote it, cleared at the start
     * of every run. A renderer drains it into its log, and a test reads it.
     *
     * Unlike reports(), nothing here is deduplicated: a printf writes a line per shading
     * point.
     **/
    const std::vector<std::string> & printed() const;

    /**
     * Which points a light shader's run lit, for the renderer to hand back to the surface
     * shader's illuminance loop.
     *
     * Every point until an `illuminate` or a `solar` narrows it, so a light shader with
     * neither, such as `ambientlight`, lights the whole batch.
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
     * An illuminance loop in progress: which light the body runs for next, the loop entry that
     * holds the lanes still going round it, and the registers the construct named. The lanes
     * live in a loop entry so that a break, a continue and a return clear them as they clear
     * any other loop's.
     **/
    class Illumination final {
     public:
        unsigned int light = 0;
        std::size_t loop = 0;
        int direction = -1;
        int colour = -1;
        std::vector<int> arguments;
    };

    /** One pass over a range of the instructions, used by both run() and initialise(). **/
    bool execute(const Program & program, std::size_t from, std::size_t until);

    bool live(unsigned int point) const;
    bool anyLive() const;
    /** Whether an instruction writing this value should write this point of it. **/
    bool writable(const Value & target, unsigned int point) const;

    void move(const Instruction & instruction);  // NOLINT(build/include_what_you_use) - the name, not std::move
    void component(const Instruction & instruction);
    void arithmetic(const Instruction & instruction);
    void compare(const Instruction & instruction);
    void product(const Instruction & instruction);
    void unary(const Instruction & instruction);
    void transform(const Instruction & instruction);  // NOLINT(build/include_what_you_use) - the name, not std::transform
    /**
     * A standard library call. Defined in Library.cxx, where every body is arithmetic over
     * the value model except the few that call the renderer.
     **/
    void builtin(const Instruction & instruction);
    /**
     * The sum of what every ambient light adds to the batch. Its own body rather than one
     * of Library.cxx's, because it is the only built-in that runs the lights itself: an
     * ambient light has no direction, so an illuminance loop skips it.
     **/
    void ambient(Value* target);
    /**
     * The registers a call writes rather than reads, from its first written argument on, or
     * none for a call that returns through its result.
     **/
    std::vector<Value*> written(const Instruction & instruction, int first);
    /**
     * The image a texture() call names, and the s and t it reads at: its own arguments, or
     * the shader's s and t when it was given only the name. Null, and reported once, when the
     * renderer cannot read it.
     **/
    const Texture* texture(const std::vector<const Value*> & given, const Value** s, const Value** t);
    /**
     * transmission and trace, the two calls the renderer computes along a line between two
     * points. When the renderer cannot do them, all the light gets through, nothing is
     * traced, and the machine reports which call failed.
     **/
    void shadowed(bool ray, const Value & from, const Value & to, Value* target);
    /**
     * The matrix into a named coordinate space. When the renderer does not recognise the space,
     * it is the identity and a report, so the scene renders in the wrong place rather than not at all.
     **/
    glm::mat4x4 space(const std::string & name);
    /** Push the lanes of the condition that are, or are not, non-zero. **/
    void mask(const Instruction & instruction, bool wanted);
    /** Narrow a loop to the lanes its condition still holds. **/
    void narrow(const Instruction & instruction);
    /**
     * Narrow the batch to the points one light reaches, for either side of the message
     * passing. False when there is no such point, and the body is skipped.
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
     * names neither contains every direction.
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
     * Where an ambient light's result lands before it is added in. Members rather than
     * locals so that a grid summing the same two lights a thousand times allocates once.
     **/
    Value direction_;
    Value colour_;
    /** The register holding P, for ambient(), which runs the lights over the batch. **/
    int point_ = -1;
    /** The registers holding s and t, which texture() reads when no coordinates are given. **/
    int s_ = -1;
    int t_ = -1;
    std::vector<char> lit_;
    std::vector<std::string> reports_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::vector<std::string> printed_;
    Renderer* renderer_ = nullptr;
    /** What prepare() sized the register file for. **/
    const Program* program_ = nullptr;
    unsigned int batch_ = 1;
    std::string error_;
};

};  // namespace v3d::render::offline::sl::runtime
