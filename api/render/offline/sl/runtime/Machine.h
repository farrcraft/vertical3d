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
 * **The execution mask is a stack.** Each shading point in the batch is a lane, and the mask
 * says which lanes are live. A condition with the same value at every point compiles to a
 * jump. A condition whose value differs runs both arms, each with only the lanes that took
 * it. An arm that no lane took is skipped. A loop iterates while any lane is live. `break`,
 * `continue` and `return` clear lanes instead of jumping, because the other lanes in the
 * batch have not finished.
 *
 * **A traced hit's batch is one point.** It runs the same program with a mask one lane wide.
 *
 * **The register file is sized once per program.** prepare() sizes it for a program and a
 * batch. A renderer then writes its inputs into the file, calls run(), and reads the results.
 * Every grid shaded with that program reuses the same registers.
 **/
class Machine final {
 public:
    /**
     * Size the register file for this program and batch, write its constants in, and keep a
     * pointer to the program for run() and initialise(). The program must outlive every later
     * run(). A batch of zero is read as one.
     **/
    void prepare(const Program & program, unsigned int batch);

    /**
     * Run the program prepare() was given. False when the run fails or nothing was prepared,
     * and error() then says why. A failure is also passed to report(), so the log shows it
     * once per machine however many runs fail the same way.
     *
     * Starts after the program's prologue. The prologue sets the declared parameter defaults,
     * and running it again would overwrite the values a scene bound.
     **/
    bool run();

    /**
     * Run only the prologue, which leaves each parameter register holding the default the
     * shader declared. `Instance::write()` calls it before writing a scene's values. A failure
     * here is not reported: `Instance::write()` reports it with the shader's name.
     **/
    bool initialise();

    /**
     * A register, by the index the program gave it. The first `Program::symbols` registers
     * hold the shader's symbols. A renderer writes P and reads Ci through them.
     **/
    Value & value(int reg);
    const Value & value(int reg) const;

    unsigned int batch() const;

    /**
     * The renderer that supplies lights, textures, named coordinate spaces and traced rays.
     * Null until one is attached. Without one, a transform to a named space leaves the value
     * unchanged and reports the space.
     **/
    void renderer(Renderer* renderer);

    const std::string & error() const;

    /**
     * Reports of what a run could not do, such as a space the renderer does not recognise
     * or a built-in with no body. Each distinct message is kept once, however many points make it.
     **/
    const std::vector<std::string> & reports() const;

    /**
     * Where a report also goes, as a warning, the first time it is made. Without one, reports
     * are only kept for reports() to read.
     **/
    void logger(const boost::shared_ptr<v3d::log::Logger> & logger);

    /**
     * Keeps a message for reports() and logs it. A message already kept is ignored.
     **/
    void report(const std::string & message);

    /**
     * The lines each `printf` in the shader wrote, in order. Cleared at the start of every
     * run. A renderer copies them into its log, and a test reads them.
     *
     * Unlike reports(), these are not deduplicated. A printf writes one line per shading
     * point.
     **/
    const std::vector<std::string> & printed() const;

    /**
     * Which points a light shader's run lit. The renderer passes this to the surface
     * shader's illuminance loop.
     *
     * Every point is lit until an `illuminate` or a `solar` narrows the set. A light shader
     * with neither, such as `ambientlight`, lights the whole batch.
     **/
    const std::vector<char> & lit() const;

 private:
    /**
     * A loop in progress: the lanes still iterating, the depth of the mask stack when the
     * loop opened, and the instruction after the loop. A break uses the depth to clear its
     * lanes from every mask pushed inside the loop.
     **/
    class Loop final {
     public:
        std::vector<char> lanes;
        std::size_t depth = 0;
        int exit = 0;
    };

    /**
     * An inlined function body in progress: the depths of the mask and loop stacks when it
     * opened. A return clears its lanes from the masks and loops above those depths, and
     * from nothing below them.
     **/
    class Frame final {
     public:
        std::size_t masks = 0;
        std::size_t loops = 0;
    };

    /**
     * An illuminance loop in progress: the next light to run the body for, the index of the
     * Loop entry that holds its lanes, and the registers for L, Cl and the construct's
     * arguments. The lanes are kept in a Loop entry so that break, continue and return clear
     * them as they do for any other loop.
     **/
    class Illumination final {
     public:
        unsigned int light = 0;
        std::size_t loop = 0;
        int direction = -1;
        int colour = -1;
        std::vector<int> arguments;
    };

    /** A run of instructions, from the first to one before the last. **/
    struct Range {
        std::size_t from = 0;
        std::size_t until = 0;
    };

    /** One pass over a range of the instructions, used by both run() and initialise(). **/
    bool execute(const Program & program, const Range & range);

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
     * A standard library call. Defined in Library.cxx, with the other built-in bodies.
     **/
    void builtin(const Instruction & instruction);
    /** Report each colour space a ctransform names that is not "rgb". **/
    void colourSpaces(const std::vector<const Value*> & given);
    /**
     * The sum of what every ambient light adds to the batch. This is the only built-in that
     * runs the lights itself. An ambient light has no direction, so an illuminance loop skips
     * it.
     **/
    void ambient(Value* target);
    /**
     * The registers a call writes rather than reads, from its first written argument on, or
     * none for a call that returns through its result.
     **/
    std::vector<Value*> written(const Instruction & instruction, int first);
    /** The registers a texture lookup reads s and t from. **/
    struct Coordinates {
        const Value* s = nullptr;
        const Value* t = nullptr;
    };

    /**
     * The image a texture() call names, and the s and t it reads at: its own arguments, or
     * the shader's s and t when it was given only the name. Null, and reported once, when the
     * renderer cannot read it.
     **/
    const Texture* texture(const std::vector<const Value*> & given, Coordinates* coordinates);
    /**
     * transmission() and trace(), which the renderer computes along a line between two
     * points. When the renderer cannot answer, transmission() lets all the light through,
     * trace() returns black, and the machine reports which call failed.
     **/
    void shadowed(bool ray, const Value & from, const Value & to, Value* target);
    /**
     * The matrix into a named coordinate space. When the renderer does not recognise the
     * space, this returns the identity and reports the name. The scene still renders, with
     * the value in the wrong place.
     **/
    glm::mat4x4 space(const std::string & name);
    /**
     * The matrix a transform built-in applies: into the space its first argument names, or
     * out of the first and into the second when it names two. `divided` is set when either
     * space is projected.
     **/
    glm::mat4x4 between(const std::vector<const Value*> & given, bool* divided);
    /** Whether the renderer divides a point in the named space by w. False without a renderer. **/
    bool projected(const std::string & name) const;
    /** Push the lanes of the condition that are, or are not, non-zero. **/
    void mask(const Instruction & instruction, bool wanted);
    /** Narrow a loop to the lanes its condition still holds. **/
    void narrow(const Instruction & instruction);
    /**
     * Narrow the batch to the points one light reaches. Runs the next round of a surface
     * shader's illuminance, or a light shader's illuminate or solar. False when the light
     * reaches no point, and the body is then skipped.
     **/
    bool admit(const Instruction & instruction);
    /**
     * Set L and Cl from the next light that reaches any point of the batch, and push the
     * lanes it reaches. False when the lights have run out.
     **/
    bool nextLight();
    /**
     * Write L for each live lane, and push the lanes that a light shader's illuminate or
     * solar lights. False when it lights none, as for a light aimed away from this batch.
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
     * Where an ambient light's result lands before it is added in. These are members so that
     * ambient() does not allocate on every call.
     **/
    Value direction_;
    Value colour_;
    /** The register holding P, which ambient() passes to each light. **/
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
