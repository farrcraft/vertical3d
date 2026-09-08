/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SLMachine.h"

#include <algorithm>
#include <string>
#include <vector>

#include <glm/geometric.hpp>

#include "SLTypes.h"

namespace v3d::render::offline {

namespace {

/**
 * A shader with a loop nothing ends would otherwise hang a render rather than fail it, and a
 * hung render says nothing about why.
 **/
const std::size_t LIMIT = 4000000;

float compare(SLOpcode opcode, float left, float right) {
    switch (opcode) {
        case SLOpcode::LESS:
            return left < right ? 1.0f : 0.0f;
        case SLOpcode::LESS_EQUAL:
            return left <= right ? 1.0f : 0.0f;
        case SLOpcode::GREATER:
            return left > right ? 1.0f : 0.0f;
        case SLOpcode::GREATER_EQUAL:
            return left >= right ? 1.0f : 0.0f;
        case SLOpcode::EQUAL:
            return left == right ? 1.0f : 0.0f;
        case SLOpcode::NOT_EQUAL:
            return left != right ? 1.0f : 0.0f;
        case SLOpcode::AND:
            return left != 0.0f && right != 0.0f ? 1.0f : 0.0f;
        default:
            return left != 0.0f || right != 0.0f ? 1.0f : 0.0f;
    }
}

float combine(SLOpcode opcode, float left, float right) {
    switch (opcode) {
        case SLOpcode::ADD:
            return left + right;
        case SLOpcode::SUBTRACT:
            return left - right;
        case SLOpcode::MULTIPLY:
            return left * right;
        default:
            return left / right;
    }
}

};  // namespace

void SLMachine::prepare(const SLProgram & program, unsigned int batch) {
    batch_ = batch == 0 ? 1 : batch;
    file_.resize(program.registers.size());
    for (std::size_t i = 0; i < program.registers.size(); i++) {
        const SLRegister & reg = program.registers[i];
        file_[i].reset(reg.type, reg.storage, batch_);
        if (!reg.constant) {
            continue;
        }
        // a constant is written once here rather than by an instruction per run
        file_[i].text(reg.text);
        for (unsigned int component = 0; component < reg.value.size(); component++) {
            file_[i].component(0, component, reg.value[component]);
        }
    }
}

SLValue & SLMachine::value(int reg) {
    return file_[static_cast<std::size_t>(reg)];
}

const SLValue & SLMachine::value(int reg) const {
    return file_[static_cast<std::size_t>(reg)];
}

unsigned int SLMachine::batch() const {
    return batch_;
}

void SLMachine::renderer(SLRenderer* renderer) {
    renderer_ = renderer;
}

const std::string & SLMachine::error() const {
    return error_;
}

const std::vector<std::string> & SLMachine::reports() const {
    return reports_;
}

void SLMachine::report(const std::string & message) {
    // once per distinct message: a 640 by 480 render would otherwise print a million lines
    // to say one thing
    if (std::ranges::find(reports_, message) == reports_.end()) {
        reports_.push_back(message);
    }
}

bool SLMachine::live(unsigned int point) const {
    return masks_.back()[point] != 0;
}

bool SLMachine::writable(const SLValue & target, unsigned int point) const {
    // a uniform value is one value for the whole batch, so it is written while any lane is
    // running rather than while lane zero is - a lane that broke out of a loop must not stop
    // a uniform counter the lanes beside it are still advancing
    return target.storage() == SLStorage::VARYING ? live(point) : anyLive();
}

bool SLMachine::anyLive() const {
    return std::ranges::any_of(masks_.back(), [](char lane) { return lane != 0; });
}

void SLMachine::move(const SLInstruction & instruction) {
    SLValue & target = file_[static_cast<std::size_t>(instruction.target)];
    const SLValue & source = file_[static_cast<std::size_t>(instruction.left)];
    if (target.type() == SLType::STRING) {
        target.text(source.text());
        return;
    }
    const unsigned int count = target.storage() == SLStorage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (writable(target, point)) {
            target.assign(source, point);
        }
    }
}

void SLMachine::component(const SLInstruction & instruction) {
    SLValue & target = file_[static_cast<std::size_t>(instruction.target)];
    const SLValue & source = file_[static_cast<std::size_t>(instruction.left)];
    const unsigned int count = target.storage() == SLStorage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (writable(target, point)) {
            target.component(point, static_cast<unsigned int>(instruction.right), source.number(point));
        }
    }
}

void SLMachine::arithmetic(const SLInstruction & instruction) {
    SLValue & target = file_[static_cast<std::size_t>(instruction.target)];
    const SLValue & left = file_[static_cast<std::size_t>(instruction.left)];
    const SLValue & right = file_[static_cast<std::size_t>(instruction.right)];
    const unsigned int count = target.storage() == SLStorage::VARYING ? batch_ : 1;
    const unsigned int wide = target.components();

    // a matrix times a matrix is the product rather than a component at a time, which is the
    // one place the shapes and the maths disagree
    if (instruction.opcode == SLOpcode::MULTIPLY &&
        left.type() == SLType::MATRIX && right.type() == SLType::MATRIX) {
        for (unsigned int point = 0; point < count; point++) {
            if (writable(target, point)) {
                target.matrix(point, left.matrix(point) * right.matrix(point));
            }
        }
        return;
    }

    for (unsigned int point = 0; point < count; point++) {
        if (!writable(target, point)) {
            continue;
        }
        for (unsigned int i = 0; i < wide; i++) {
            // a one component operand is a scale over the whole of the other, which is RI's
            // promotion rather than a zero fill
            const float a = left.component(point, left.components() == 1 ? 0 : i);
            const float b = right.component(point, right.components() == 1 ? 0 : i);
            target.component(point, i, combine(instruction.opcode, a, b));
        }
    }
}

void SLMachine::compare(const SLInstruction & instruction) {
    SLValue & target = file_[static_cast<std::size_t>(instruction.target)];
    const SLValue & left = file_[static_cast<std::size_t>(instruction.left)];
    const SLValue & right = file_[static_cast<std::size_t>(instruction.right)];
    const unsigned int count = target.storage() == SLStorage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (writable(target, point)) {
            target.number(point, offline::compare(instruction.opcode,
                left.number(point), right.number(point)));
        }
    }
}

void SLMachine::product(const SLInstruction & instruction) {
    SLValue & target = file_[static_cast<std::size_t>(instruction.target)];
    const SLValue & left = file_[static_cast<std::size_t>(instruction.left)];
    const SLValue & right = file_[static_cast<std::size_t>(instruction.right)];
    const unsigned int count = target.storage() == SLStorage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (!writable(target, point)) {
            continue;
        }
        if (instruction.opcode == SLOpcode::DOT) {
            target.number(point, glm::dot(left.triple(point), right.triple(point)));
        } else {
            target.triple(point, glm::cross(left.triple(point), right.triple(point)));
        }
    }
}

void SLMachine::unary(const SLInstruction & instruction) {
    SLValue & target = file_[static_cast<std::size_t>(instruction.target)];
    const SLValue & source = file_[static_cast<std::size_t>(instruction.left)];
    const unsigned int count = target.storage() == SLStorage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (!writable(target, point)) {
            continue;
        }
        if (instruction.opcode == SLOpcode::NOT) {
            target.number(point, source.number(point) == 0.0f ? 1.0f : 0.0f);
            continue;
        }
        for (unsigned int i = 0; i < target.components(); i++) {
            target.component(point, i, -source.component(point, i));
        }
    }
}

void SLMachine::transform(const SLInstruction & instruction) {
    SLValue & target = file_[static_cast<std::size_t>(instruction.target)];
    const SLValue & source = file_[static_cast<std::size_t>(instruction.left)];
    const std::string & space = file_[static_cast<std::size_t>(instruction.right)].text();

    glm::mat4x4 matrix(1.0f);
    if (renderer_ == nullptr || !renderer_->space(space, &matrix)) {
        // the value still arrives, in the space it was already in: a scene that named a space
        // nothing knows renders in the wrong place rather than not at all, and says so
        report("the coordinate space \"" + space + "\" is not one this renderer knows");
    }
    const unsigned int count = target.storage() == SLStorage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (!writable(target, point)) {
            continue;
        }
        switch (target.type()) {
            case SLType::POINT:
                target.triple(point, ptransform(matrix, source.triple(point)));
                break;
            case SLType::VECTOR:
                target.triple(point, vtransform(matrix, source.triple(point)));
                break;
            case SLType::NORMAL:
                target.triple(point, ntransform(matrix, source.triple(point)));
                break;
            default:
                // a colour space and a matrix space are ctransform's and mtransform's
                target.assign(source, point);
                break;
        }
    }
}

void SLMachine::mask(const SLInstruction & instruction, bool wanted) {
    const SLValue & condition = file_[static_cast<std::size_t>(instruction.left)];
    std::vector<char> next = masks_.back();
    for (unsigned int point = 0; point < batch_; point++) {
        if (next[point] != 0 && (condition.number(point) != 0.0f) != wanted) {
            next[point] = 0;
        }
    }
    masks_.push_back(next);
}

void SLMachine::narrow(const SLInstruction & instruction) {
    Loop & loop = loops_.back();
    const SLValue & condition = file_[static_cast<std::size_t>(instruction.left)];
    for (unsigned int point = 0; point < batch_; point++) {
        if (loop.lanes[point] != 0 && condition.number(point) == 0.0f) {
            loop.lanes[point] = 0;
        }
    }
    masks_.back() = loop.lanes;
}

void SLMachine::finish() {
    // these lanes are done with the shader, so they come out of every mask and out of every
    // loop still going round them
    const std::vector<char> going = masks_.back();
    for (std::vector<char> & mask : masks_) {
        for (unsigned int point = 0; point < batch_; point++) {
            if (going[point] != 0) {
                mask[point] = 0;
            }
        }
    }
    for (Loop & loop : loops_) {
        for (unsigned int point = 0; point < batch_; point++) {
            if (going[point] != 0) {
                loop.lanes[point] = 0;
            }
        }
    }
}

void SLMachine::leave(bool loop) {
    if (loops_.empty()) {
        report("a 'break' or a 'continue' outside a loop does nothing");
        return;
    }
    Loop & current = loops_.back();
    const std::vector<char> going = masks_.back();
    // a lane leaves by losing its bit in every mask inside the loop, because the lanes beside
    // it have not finished and there is nowhere to jump to on its behalf
    for (std::size_t i = current.depth; i < masks_.size(); i++) {
        for (unsigned int point = 0; point < batch_; point++) {
            if (going[point] != 0) {
                masks_[i][point] = 0;
            }
        }
    }
    if (!loop) {
        return;
    }
    for (unsigned int point = 0; point < batch_; point++) {
        if (going[point] != 0) {
            current.lanes[point] = 0;
        }
    }
}

bool SLMachine::run(const SLProgram & program) {
    error_.clear();
    masks_.assign(1, std::vector<char>(batch_, 1));
    loops_.clear();

    std::size_t pc = 0;
    std::size_t steps = 0;
    while (pc < program.instructions.size()) {
        if (++steps > LIMIT) {
            error_ = "the shader '" + program.name + "' ran without end";
            return false;
        }
        const SLInstruction & instruction = program.instructions[pc];
        switch (instruction.opcode) {
            case SLOpcode::MOVE:
                move(instruction);  // NOLINT(build/include_what_you_use) - the name, not std::move
                break;
            case SLOpcode::MOVE_COMPONENT:
                component(instruction);
                break;
            case SLOpcode::ADD:
            case SLOpcode::SUBTRACT:
            case SLOpcode::MULTIPLY:
            case SLOpcode::DIVIDE:
                arithmetic(instruction);
                break;
            case SLOpcode::NEGATE:
            case SLOpcode::NOT:
                unary(instruction);
                break;
            case SLOpcode::DOT:
            case SLOpcode::CROSS:
                product(instruction);
                break;
            case SLOpcode::LESS:
            case SLOpcode::LESS_EQUAL:
            case SLOpcode::GREATER:
            case SLOpcode::GREATER_EQUAL:
            case SLOpcode::EQUAL:
            case SLOpcode::NOT_EQUAL:
            case SLOpcode::AND:
            case SLOpcode::OR:
                compare(instruction);
                break;
            case SLOpcode::TRANSFORM:
                transform(instruction);
                break;
            case SLOpcode::CALL:
                // the standard library is what a call runs, and it is handed to the machine
                // rather than held by it
                report("the standard library is not attached, so a call answers its default");
                break;
            case SLOpcode::JUMP:
                pc = static_cast<std::size_t>(instruction.target);
                continue;
            case SLOpcode::JUMP_IF_ZERO:
                if (file_[static_cast<std::size_t>(instruction.left)].number(0) == 0.0f) {
                    pc = static_cast<std::size_t>(instruction.target);
                    continue;
                }
                break;
            case SLOpcode::MASK:
            case SLOpcode::MASK_NOT:
                mask(instruction, instruction.opcode == SLOpcode::MASK);
                // an arm no lane took costs the push and nothing else
                if (!anyLive()) {
                    pc = static_cast<std::size_t>(instruction.target);
                    continue;
                }
                break;
            case SLOpcode::POP_MASK:
                masks_.pop_back();
                break;
            case SLOpcode::LOOP: {
                Loop loop;
                loop.lanes = masks_.back();
                loop.depth = masks_.size();
                loop.exit = instruction.target;
                loops_.push_back(loop);
                masks_.push_back(loops_.back().lanes);
                break;
            }
            case SLOpcode::LOOP_TEST:
                narrow(instruction);
                if (!anyLive()) {
                    pc = static_cast<std::size_t>(loops_.back().exit);
                    continue;
                }
                break;
            case SLOpcode::LOOP_RESTORE:
                masks_.back() = loops_.back().lanes;
                if (!anyLive()) {
                    pc = static_cast<std::size_t>(loops_.back().exit);
                    continue;
                }
                break;
            case SLOpcode::LOOP_END:
                // a lane that took a continue comes back for the next pass; one that broke
                // does not, because break cleared it from the loop's own lanes
                masks_.back() = loops_.back().lanes;
                pc = static_cast<std::size_t>(anyLive() ? instruction.target : loops_.back().exit);
                continue;
            case SLOpcode::POP_LOOP:
                masks_.pop_back();
                loops_.pop_back();
                break;
            case SLOpcode::BREAK:
                leave(true);
                break;
            case SLOpcode::CONTINUE:
                leave(false);
                break;
            case SLOpcode::RETURN:
                finish();
                break;
        }
        pc++;
    }
    return true;
}

};  // namespace v3d::render::offline
