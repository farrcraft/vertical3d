/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Machine.h"

#include <api/render/offline/sl/Types.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::sl::runtime {

namespace {

/**
 * The most instructions one run executes before it fails. Without a limit, a shader with an
 * endless loop would hang a render rather than fail it, and give no reason.
 **/
const std::size_t LIMIT = 4000000;

float compare(Opcode opcode, float left, float right) {
    switch (opcode) {
        case Opcode::LESS:
            return left < right ? 1.0f : 0.0f;
        case Opcode::LESS_EQUAL:
            return left <= right ? 1.0f : 0.0f;
        case Opcode::GREATER:
            return left > right ? 1.0f : 0.0f;
        case Opcode::GREATER_EQUAL:
            return left >= right ? 1.0f : 0.0f;
        case Opcode::EQUAL:
            return left == right ? 1.0f : 0.0f;
        case Opcode::NOT_EQUAL:
            return left != right ? 1.0f : 0.0f;
        case Opcode::AND:
            return left != 0.0f && right != 0.0f ? 1.0f : 0.0f;
        default:
            return left != 0.0f || right != 0.0f ? 1.0f : 0.0f;
    }
}

/**
 * Component i of a value read as one of wide components. A float fills every component, except
 * that a float read as a matrix is the diagonal matrix, which is how an assignment promotes it.
 **/
float promoted(const Value & value, unsigned int point, unsigned int i, unsigned int wide) {
    if (value.components() != 1) {
        return value.component(point, i);
    }
    if (wide == 16 && i % 5 != 0) {
        return 0.0f;
    }
    return value.number(point);
}

float combine(Opcode opcode, float left, float right) {
    switch (opcode) {
        case Opcode::ADD:
            return left + right;
        case Opcode::SUBTRACT:
            return left - right;
        case Opcode::MULTIPLY:
            return left * right;
        default:
            return left / right;
    }
}

};  // namespace

void Machine::prepare(const Program & program, unsigned int batch) {
    program_ = &program;
    batch_ = batch == 0 ? 1 : batch;
    point_ = program.symbol("P");
    s_ = program.symbol("s");
    t_ = program.symbol("t");
    direction_.reset(Type::VECTOR, Storage::VARYING, batch_);
    colour_.reset(Type::COLOR, Storage::VARYING, batch_);
    file_.resize(program.registers.size());
    for (std::size_t i = 0; i < program.registers.size(); i++) {
        const Register & reg = program.registers[i];
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

Value & Machine::value(int reg) {
    return file_[static_cast<std::size_t>(reg)];
}

const Value & Machine::value(int reg) const {
    return file_[static_cast<std::size_t>(reg)];
}

unsigned int Machine::batch() const {
    return batch_;
}

void Machine::renderer(Renderer* renderer) {
    renderer_ = renderer;
}

const std::string & Machine::error() const {
    return error_;
}

const std::vector<std::string> & Machine::reports() const {
    return reports_;
}

const std::vector<std::string> & Machine::printed() const {
    return printed_;
}

const std::vector<char> & Machine::lit() const {
    return lit_;
}

void Machine::report(const std::string & message) {
    // once per distinct message: a 640 by 480 render would otherwise print a million lines
    // to say one thing
    if (std::ranges::find(reports_, message) == reports_.end()) {
        reports_.push_back(message);
        if (logger_) {
            logger_->get()->warn("shader: {}", message);
        }
    }
}

void Machine::logger(const boost::shared_ptr<v3d::log::Logger> & logger) {
    logger_ = logger;
}

bool Machine::live(unsigned int point) const {
    return masks_.back()[point] != 0;
}

bool Machine::writable(const Value & target, unsigned int point) const {
    // a uniform value is one value for the whole batch, so it is written while any lane is
    // running rather than while lane zero is - a lane that broke out of a loop must not stop
    // a uniform counter the lanes beside it are still advancing
    return target.storage() == Storage::VARYING ? live(point) : anyLive();
}

bool Machine::anyLive() const {
    return std::ranges::any_of(masks_.back(), [](char lane) { return lane != 0; });
}

void Machine::move(const Instruction & instruction) {
    Value & target = file_[static_cast<std::size_t>(instruction.target)];
    const Value & source = file_[static_cast<std::size_t>(instruction.left)];
    if (target.type() == Type::STRING) {
        target.text(source.text());
        return;
    }
    const unsigned int count = target.storage() == Storage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (writable(target, point)) {
            target.assign(source, point);
        }
    }
}

void Machine::component(const Instruction & instruction) {
    Value & target = file_[static_cast<std::size_t>(instruction.target)];
    const Value & source = file_[static_cast<std::size_t>(instruction.left)];
    const unsigned int count = target.storage() == Storage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (writable(target, point)) {
            target.component(point, static_cast<unsigned int>(instruction.right), source.number(point));
        }
    }
}

void Machine::arithmetic(const Instruction & instruction) {
    Value & target = file_[static_cast<std::size_t>(instruction.target)];
    const Value & left = file_[static_cast<std::size_t>(instruction.left)];
    const Value & right = file_[static_cast<std::size_t>(instruction.right)];
    const unsigned int count = target.storage() == Storage::VARYING ? batch_ : 1;
    const unsigned int wide = target.components();

    // a matrix times a matrix is the matrix product, and a matrix over a matrix is the left
    // times the inverse of the right. SL states both in row vectors, so A * B applies A and
    // then B. A glm matrix applies to a column vector on its right, so the order is reversed
    if ((instruction.opcode == Opcode::MULTIPLY || instruction.opcode == Opcode::DIVIDE) &&
        left.type() == Type::MATRIX && right.type() == Type::MATRIX) {
        for (unsigned int point = 0; point < count; point++) {
            if (!writable(target, point)) {
                continue;
            }
            const glm::mat4x4 second = instruction.opcode == Opcode::MULTIPLY ?
                right.matrix(point) : glm::inverse(right.matrix(point));
            target.matrix(point, second * left.matrix(point));
        }
        return;
    }

    for (unsigned int point = 0; point < count; point++) {
        if (!writable(target, point)) {
            continue;
        }
        for (unsigned int i = 0; i < wide; i++) {
            // a one component operand is a scale over the whole of the other, as RI promotes
            // it, rather than a zero fill
            const float a = left.component(point, left.components() == 1 ? 0 : i);
            const float b = right.component(point, right.components() == 1 ? 0 : i);
            target.component(point, i, combine(instruction.opcode, a, b));
        }
    }
}

void Machine::compare(const Instruction & instruction) {
    Value & target = file_[static_cast<std::size_t>(instruction.target)];
    const Value & left = file_[static_cast<std::size_t>(instruction.left)];
    const Value & right = file_[static_cast<std::size_t>(instruction.right)];
    const unsigned int count = target.storage() == Storage::VARYING ? batch_ : 1;
    // a string holds no number, so two of them are equal or not by their text
    const bool text = left.type() == Type::STRING && right.type() == Type::STRING;
    const float same = left.text() == right.text() ? 1.0f : 0.0f;
    // two colours, points or matrices are equal when every component is; a float on one side
    // is promoted as an assignment promotes it, so a matrix compares against a diagonal
    const bool whole = (instruction.opcode == Opcode::EQUAL || instruction.opcode == Opcode::NOT_EQUAL) &&
        (left.components() > 1 || right.components() > 1);
    const unsigned int wide = std::max(left.components(), right.components());
    for (unsigned int point = 0; point < count; point++) {
        if (!writable(target, point)) {
            continue;
        }
        if (text) {
            target.number(point, instruction.opcode == Opcode::NOT_EQUAL ? 1.0f - same : same);
        } else if (whole) {
            bool equal = true;
            for (unsigned int i = 0; i < wide && equal; i++) {
                equal = promoted(left, point, i, wide) == promoted(right, point, i, wide);
            }
            target.number(point, equal == (instruction.opcode == Opcode::EQUAL) ? 1.0f : 0.0f);
        } else {
            target.number(point, runtime::compare(instruction.opcode,
                left.number(point), right.number(point)));
        }
    }
}

void Machine::product(const Instruction & instruction) {
    Value & target = file_[static_cast<std::size_t>(instruction.target)];
    const Value & left = file_[static_cast<std::size_t>(instruction.left)];
    const Value & right = file_[static_cast<std::size_t>(instruction.right)];
    const unsigned int count = target.storage() == Storage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (!writable(target, point)) {
            continue;
        }
        if (instruction.opcode == Opcode::DOT) {
            target.number(point, glm::dot(left.triple(point), right.triple(point)));
        } else {
            target.triple(point, glm::cross(left.triple(point), right.triple(point)));
        }
    }
}

void Machine::unary(const Instruction & instruction) {
    Value & target = file_[static_cast<std::size_t>(instruction.target)];
    const Value & source = file_[static_cast<std::size_t>(instruction.left)];
    const unsigned int count = target.storage() == Storage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (!writable(target, point)) {
            continue;
        }
        if (instruction.opcode == Opcode::NOT) {
            target.number(point, source.number(point) == 0.0f ? 1.0f : 0.0f);
            continue;
        }
        for (unsigned int i = 0; i < target.components(); i++) {
            target.component(point, i, -source.component(point, i));
        }
    }
}

glm::mat4x4 Machine::space(const std::string & name) {
    glm::mat4x4 matrix(1.0f);
    if (renderer_ == nullptr || !renderer_->space(name, &matrix)) {
        // the value still arrives, unchanged in the space it was already in
        report("the coordinate space \"" + name + "\" is not one this renderer knows");
    }
    return matrix;
}

void Machine::transform(const Instruction & instruction) {
    Value & target = file_[static_cast<std::size_t>(instruction.target)];
    const Value & source = file_[static_cast<std::size_t>(instruction.left)];
    // only a position or a direction is in a coordinate space, so nothing else asks the
    // renderer for one. A cast states its value in the named space and the shader works in
    // the current one, so the cast applies the inverse of the matrix the renderer returns
    const bool geometric = pointlike(target.type());
    const glm::mat4x4 matrix = geometric ?
        glm::inverse(space(file_[static_cast<std::size_t>(instruction.right)].text())) : glm::mat4x4(1.0f);
    const unsigned int count = target.storage() == Storage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (!writable(target, point)) {
            continue;
        }
        switch (target.type()) {
            case Type::POINT:
                target.triple(point, ptransform(matrix, source.triple(point)));
                break;
            case Type::VECTOR:
                target.triple(point, vtransform(matrix, source.triple(point)));
                break;
            case Type::NORMAL:
                target.triple(point, ntransform(matrix, source.triple(point)));
                break;
            case Type::VOID:
            case Type::FLOAT:
            case Type::COLOR:
            case Type::MATRIX:
            case Type::STRING:
                // the compiler sends a colour's space to ctransform and rejects a space on
                // any other type, so the value is copied unchanged
                target.assign(source, point);
                break;
        }
    }
}

bool Machine::inside(const glm::vec3 & direction, const std::vector<int> & cone,
    std::size_t first, unsigned int point) const {
    if (cone.size() < first + 2) {
        return true;
    }
    const glm::vec3 axis = file_[static_cast<std::size_t>(cone[first])].triple(point);
    const float angle = file_[static_cast<std::size_t>(cone[first + 1])].number(point);
    const float length = glm::length(direction) * glm::length(axis);
    if (length == 0.0f) {
        return true;
    }
    return glm::dot(direction, axis) / length >= std::cos(angle);
}

bool Machine::nextLight() {
    Illumination & round = illuminations_.back();
    const unsigned int count = renderer_ == nullptr ? 0 : renderer_->lights();
    const Value & surface = file_[static_cast<std::size_t>(round.arguments[0])];
    Value & direction = file_[static_cast<std::size_t>(round.direction)];
    Value & colour = file_[static_cast<std::size_t>(round.colour)];

    while (round.light < count) {
        const unsigned int index = round.light++;
        std::vector<char> reached(batch_, 1);
        bool ambient = false;
        if (!renderer_->light(index, surface, &direction, &colour, &reached, &ambient) || ambient) {
            // an illuminance loop skips an ambient light: it has no direction to test against
            // the cone, and ambient() sums it instead
            continue;
        }
        std::vector<char> lanes = loops_[round.loop].lanes;
        bool any = false;
        for (unsigned int point = 0; point < batch_; point++) {
            if (lanes[point] == 0) {
                continue;
            }
            // the cone opens along the axis from the point being shaded, and L points at
            // the light, so it is L itself that is tested rather than the way light travels
            if (reached[point] == 0 || !inside(direction.triple(point), round.arguments, 1, point)) {
                lanes[point] = 0;
                continue;
            }
            any = true;
        }
        if (any) {
            masks_.push_back(lanes);
            return true;
        }
    }
    return false;
}

bool Machine::admit(const Instruction & instruction) {
    if (instruction.opcode == Opcode::ILLUMINANCE_NEXT) {
        return nextLight();
    }
    return illuminate(instruction, instruction.opcode == Opcode::SOLAR);
}

bool Machine::illuminate(const Instruction & instruction, bool solar) {
    Value & direction = file_[static_cast<std::size_t>(instruction.left)];
    const Value & surface = file_[static_cast<std::size_t>(instruction.right)];
    const std::vector<int> & given = instruction.arguments;
    std::vector<char> lanes = masks_.back();
    bool any = false;

    for (unsigned int point = 0; point < batch_; point++) {
        if (lanes[point] == 0) {
            continue;
        }
        glm::vec3 toward(0.0f);
        if (given.empty()) {
            // solar with no axis is light from every direction at once, which has no L
            direction.triple(point, toward);
        } else if (solar) {
            // the argument is the way the light travels, and L points back along it
            toward = -file_[static_cast<std::size_t>(given[0])].triple(point);
            direction.triple(point, toward);
            // an angle lets L be any direction inside a cone, chosen against the surface's own
            // cone, which a light shader is not given. The light comes along its axis instead
            if (given.size() > 1 && file_[static_cast<std::size_t>(given[1])].number(point) != 0.0f) {
                report("solar with an angle is lit along its axis only, as if the angle were 0");
            }
        } else {
            toward = file_[static_cast<std::size_t>(given[0])].triple(point) - surface.triple(point);
            direction.triple(point, toward);
        }
        // a light's own cone is stated from the light outward, so it is the way the light
        // travels that is tested rather than L
        if (!solar && !inside(-toward, given, 1, point)) {
            lanes[point] = 0;
            continue;
        }
        any = true;
    }
    for (unsigned int point = 0; point < batch_; point++) {
        if (lanes[point] == 0) {
            lit_[point] = 0;
        }
    }
    if (any) {
        masks_.push_back(lanes);
    }
    return any;
}

void Machine::mask(const Instruction & instruction, bool wanted) {
    const Value & condition = file_[static_cast<std::size_t>(instruction.left)];
    std::vector<char> next = masks_.back();
    for (unsigned int point = 0; point < batch_; point++) {
        if (next[point] != 0 && (condition.number(point) != 0.0f) != wanted) {
            next[point] = 0;
        }
    }
    masks_.push_back(next);
}

void Machine::narrow(const Instruction & instruction) {
    Loop & loop = loops_.back();
    const Value & condition = file_[static_cast<std::size_t>(instruction.left)];
    for (unsigned int point = 0; point < batch_; point++) {
        if (loop.lanes[point] != 0 && condition.number(point) == 0.0f) {
            loop.lanes[point] = 0;
        }
    }
    masks_.back() = loop.lanes;
}

void Machine::finish() {
    // a return goes as far as the function it is in and no further, so the stacks are
    // cleared from where that body opened rather than from the bottom
    const std::size_t mask = frames_.empty() ? 0 : frames_.back().masks;
    const std::size_t loop = frames_.empty() ? 0 : frames_.back().loops;
    const std::vector<char> going = masks_.back();
    for (std::size_t i = mask; i < masks_.size(); i++) {
        for (unsigned int point = 0; point < batch_; point++) {
            if (going[point] != 0) {
                masks_[i][point] = 0;
            }
        }
    }
    for (std::size_t i = loop; i < loops_.size(); i++) {
        for (unsigned int point = 0; point < batch_; point++) {
            if (going[point] != 0) {
                loops_[i].lanes[point] = 0;
            }
        }
    }
}

void Machine::leave(bool loop) {
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

bool Machine::initialise() {
    if (program_ == nullptr) {
        error_ = "the machine was run before it was prepared";
        return false;
    }
    return execute(*program_, 0, program_->prologue);
}

bool Machine::run() {
    if (program_ == nullptr) {
        error_ = "the machine was run before it was prepared";
        report(error_);
        return false;
    }
    if (!execute(*program_, program_->prologue, program_->instructions.size())) {
        // reported here, so no renderer has to log a failed run itself; initialise() does not
        // report, because Instance::write reports it with the shader's name
        report(error_);
        return false;
    }
    return true;
}

bool Machine::execute(const Program & program, std::size_t from, std::size_t until) {
    error_.clear();
    printed_.clear();
    masks_.assign(1, std::vector<char>(batch_, 1));
    loops_.clear();
    frames_.clear();
    illuminations_.clear();
    // every point until an illuminate or a solar narrows it, so a light shader with neither
    // lights the whole batch
    lit_.assign(batch_, 1);

    std::size_t pc = from;
    std::size_t steps = 0;
    while (pc < until) {
        if (++steps > LIMIT) {
            error_ = "the shader '" + program.name + "' ran without end";
            return false;
        }
        const Instruction & instruction = program.instructions[pc];
        switch (instruction.opcode) {
            case Opcode::MOVE:
                move(instruction);  // NOLINT(build/include_what_you_use) - the name, not std::move
                break;
            case Opcode::MOVE_COMPONENT:
                component(instruction);
                break;
            case Opcode::ADD:
            case Opcode::SUBTRACT:
            case Opcode::MULTIPLY:
            case Opcode::DIVIDE:
                arithmetic(instruction);
                break;
            case Opcode::NEGATE:
            case Opcode::NOT:
                unary(instruction);
                break;
            case Opcode::DOT:
            case Opcode::CROSS:
                product(instruction);
                break;
            case Opcode::LESS:
            case Opcode::LESS_EQUAL:
            case Opcode::GREATER:
            case Opcode::GREATER_EQUAL:
            case Opcode::EQUAL:
            case Opcode::NOT_EQUAL:
            case Opcode::AND:
            case Opcode::OR:
                compare(instruction);
                break;
            case Opcode::TRANSFORM:
                transform(instruction);
                break;
            case Opcode::CALL:
                builtin(instruction);
                break;
            case Opcode::JUMP:
                pc = static_cast<std::size_t>(instruction.target);
                continue;
            case Opcode::JUMP_IF_ZERO:
                if (file_[static_cast<std::size_t>(instruction.left)].number(0) == 0.0f) {
                    pc = static_cast<std::size_t>(instruction.target);
                    continue;
                }
                break;
            case Opcode::MASK:
            case Opcode::MASK_NOT:
                mask(instruction, instruction.opcode == Opcode::MASK);
                // an arm no lane took costs the push and nothing else
                if (!anyLive()) {
                    pc = static_cast<std::size_t>(instruction.target);
                    continue;
                }
                break;
            case Opcode::POP_MASK:
                masks_.pop_back();
                break;
            case Opcode::LOOP: {
                Loop loop;
                loop.lanes = masks_.back();
                loop.depth = masks_.size();
                loop.exit = instruction.target;
                loops_.push_back(loop);
                masks_.push_back(loops_.back().lanes);
                break;
            }
            case Opcode::LOOP_TEST:
                narrow(instruction);
                if (!anyLive()) {
                    pc = static_cast<std::size_t>(loops_.back().exit);
                    continue;
                }
                break;
            case Opcode::LOOP_RESTORE:
                masks_.back() = loops_.back().lanes;
                if (!anyLive()) {
                    pc = static_cast<std::size_t>(loops_.back().exit);
                    continue;
                }
                break;
            case Opcode::LOOP_END:
                // a lane that took a continue comes back for the next iteration; one that broke
                // does not, because break cleared it from the loop's own lanes
                masks_.back() = loops_.back().lanes;
                pc = static_cast<std::size_t>(anyLive() ? instruction.target : loops_.back().exit);
                continue;
            case Opcode::POP_LOOP:
                masks_.pop_back();
                loops_.pop_back();
                break;
            case Opcode::BREAK:
                leave(true);
                break;
            case Opcode::CONTINUE:
                leave(false);
                break;
            case Opcode::ENTER: {
                Frame frame;
                frame.masks = masks_.size();
                frame.loops = loops_.size();
                frames_.push_back(frame);
                masks_.push_back(masks_.back());
                break;
            }
            case Opcode::LEAVE:
                // the lanes that returned are live again: they are done with the function,
                // not with the shader
                masks_.resize(frames_.back().masks);
                loops_.resize(frames_.back().loops);
                frames_.pop_back();
                break;
            case Opcode::ILLUMINANCE: {
                Loop loop;
                loop.lanes = masks_.back();
                loop.depth = masks_.size();
                loop.exit = instruction.target;
                loops_.push_back(loop);
                Illumination round;
                round.loop = loops_.size() - 1;
                round.direction = instruction.left;
                round.colour = instruction.right;
                round.arguments = instruction.arguments;
                illuminations_.push_back(round);
                break;
            }
            case Opcode::ILLUMINANCE_NEXT:
            case Opcode::ILLUMINATE:
            case Opcode::SOLAR:
                // all three narrow the batch to the points one light reaches, and all
                // three skip the body when the light reaches none of them
                if (!admit(instruction)) {
                    pc = static_cast<std::size_t>(instruction.target);
                    continue;
                }
                break;
            case Opcode::POP_ILLUMINANCE:
                illuminations_.pop_back();
                loops_.pop_back();
                break;
            case Opcode::RETURN:
                // like a break it does not jump: the masks and the loops between here and
                // the body's own instruction are what has to be unwound, and each of them
                // unwinds itself once no lane is left inside it
                finish();
                break;
        }
        pc++;
    }
    return true;
}

};  // namespace v3d::render::offline::sl::runtime
