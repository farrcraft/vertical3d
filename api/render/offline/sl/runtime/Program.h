/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Types.h>

#include <string>
#include <vector>

namespace v3d::render::offline::sl::runtime {

/**
 * What one instruction does.
 *
 * A flat list over register indices rather than a tree the machine walks, for two reasons:
 * the mask handling below wants somewhere to put a jump, and a tree walk over a batch
 * allocates at every node.
 **/
enum class Opcode {
    /** target = left, converting as the two registers' types call for. **/
    MOVE,
    /**
     * One component of target takes the number in left, `right` saying which. A
     * parenthesised list is a literal for the type in front of it, and this is how each of
     * its values reaches its own component.
     **/
    MOVE_COMPONENT,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    /** target = -left. **/
    NEGATE,
    /** target = left == 0. **/
    NOT,
    /** The dot product, which SL writes '.' - a number out of two directions. **/
    DOT,
    /** The cross product, which SL writes '^'. **/
    CROSS,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    EQUAL,
    NOT_EQUAL,
    AND,
    OR,
    /**
     * A standard library function: `left` is its index in builtins() and `arguments` are
     * the registers holding what it was given. What it does is the library's, which the
     * machine is handed rather than holds.
     **/
    CALL,
    /**
     * A named coordinate space: target = left through the matrix the renderer answers for
     * the space in the string register `right`, treating it as the target register's type -
     * a point translates, a vector does not, a normal goes by the inverse transpose.
     **/
    TRANSFORM,
    /** Jump to `target`. **/
    JUMP,
    /**
     * Jump to `target` when the **uniform** register `left` is zero. A condition every point
     * agrees about is a jump rather than a mask, which is the optimisation that makes the
     * common case free.
     **/
    JUMP_IF_ZERO,
    /**
     * Push the lanes of `left` that are not zero, and jump to `target` when none are - an
     * arm no point takes costs the push and nothing else.
     **/
    MASK,
    /** The same for the lanes that are zero: the else arm. **/
    MASK_NOT,
    POP_MASK,
    /**
     * Begin a loop. `target` is the address of its POP_LOOP, which is where a test that
     * runs out of lanes jumps to.
     **/
    LOOP,
    /**
     * Narrow the loop to the lanes where `left` is not zero, and leave it when none are.
     **/
    LOOP_TEST,
    /**
     * Put the loop's lanes back without leaving the body: what a for loop puts in front of
     * its step, since a continue goes to the step rather than past it.
     **/
    LOOP_RESTORE,
    /** The bottom of a loop body: put the loop's lanes back and jump to `target`. **/
    LOOP_END,
    POP_LOOP,
    /**
     * Take the live lanes out of the loop for good, or for the rest of this pass round it.
     * Neither jumps: a lane leaves by losing its bit, because the lanes beside it have not
     * finished.
     **/
    BREAK,
    CONTINUE,
    /**
     * Open an inlined function body.
     *
     * A call is inlined because a run has no call stack, and the body is bracketed rather
     * than merely pasted in because a `return` inside it means "these lanes are done with
     * this function", not "done with this shader".
     **/
    ENTER,
    /** Close an inlined body: the lanes that returned from it are live again. **/
    LEAVE,
    /**
     * Open an illuminance loop. `left` and `right` are the surface shader's L and Cl, and
     * `arguments` are the construct's own - the point being shaded, and a cone axis and
     * half angle when it named one.
     *
     * The message passing of ADR-0026, from the side the surface is on: the body runs once
     * per light, over the same batch, with L and Cl set by that light's own program.
     **/
    ILLUMINANCE,
    /**
     * Set L and Cl from the next light the body has not run for, and narrow the batch to
     * the points that light reaches inside the cone. Jump to `target` when the lights run
     * out - and skip a light no point of this batch sees, which is what keeps a scene with
     * a hundred lights from running the body a hundred times over an empty mask.
     **/
    ILLUMINANCE_NEXT,
    POP_ILLUMINANCE,
    /**
     * The other end of the message passing, in a light shader with a position: L is from
     * the point being lit toward `arguments[0]`, and the batch narrows to the points inside
     * the cone `arguments[1]` and `arguments[2]` name. `left` and `right` are the light
     * shader's L and Ps. Jump to `target` when the light reaches no point at all.
     **/
    ILLUMINATE,
    /**
     * The same for a light at infinity, where every point is lit from one direction: L is
     * against `arguments[0]`, which is the direction the light travels in.
     **/
    SOLAR,
    /**
     * Take the live lanes out of the innermost inlined body, or out of everything when the
     * shader body is what is running. Like BREAK it does not jump, because the lanes beside
     * it have not finished.
     **/
    RETURN
};

class Instruction final {
 public:
    Opcode opcode = Opcode::MOVE;
    /** The register written, or the address jumped to. **/
    int target = -1;
    int left = -1;
    int right = -1;
    /** A call's arguments, and nothing else's. **/
    std::vector<int> arguments;
    /** Where in the source this came from, for a runtime report. **/
    unsigned int line = 0;
    unsigned int column = 0;
};

/**
 * One value the program holds, sized once when the machine is prepared.
 **/
class Register final {
 public:
    Type type = Type::FLOAT;
    Storage storage = Storage::UNIFORM;
    /** The symbol's name, or empty for a temporary. **/
    std::string name;
    /**
     * Whether the register holds a literal the machine writes once at the start of a run
     * rather than something the program computes.
     **/
    bool constant = false;
    std::vector<float> value;
    std::string text;
};

/**
 * A compiled shader: its registers and the instructions over them.
 *
 * The first `symbols` registers are the compiler's symbols in its own order - globals, then
 * the shader's parameters, then the locals - so a renderer binds a parameter or reads Ci by
 * the index the compiler gave it. Everything after them is a temporary or a constant.
 **/
class Program final {
 public:
    ShaderType type = ShaderType::SURFACE;
    std::string name;
    std::vector<Register> registers;
    std::vector<Instruction> instructions;
    /** How many of the registers are symbols. **/
    std::size_t symbols = 0;

    /**
     * The register a named symbol is, or -1. What a renderer binds a parameter or reads Ci
     * through, when it did not keep the index the compiler gave it.
     **/
    int symbol(const std::string & wanted) const;
};

};  // namespace v3d::render::offline::sl::runtime
