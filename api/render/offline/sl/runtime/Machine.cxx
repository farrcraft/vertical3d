/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Machine.h"

#include <api/render/offline/sl/Types.h>

#include "Instruction.h"
#include "Program.h"
#include "Register.h"
#include "Renderer.h"
#include "Value.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::sl::runtime {

namespace {

/**
 * The most instructions one run executes before it fails. A shader with an endless loop then
 * fails with a reason instead of hanging the render.
 **/
const std::size_t LIMIT = 4000000;

float compare(Opcode opcode, float left, float right) {
    switch (opcode) {
        case Opcode::LESS:
            return left < right ? 1.0F : 0.0F;
        case Opcode::LESS_EQUAL:
            return left <= right ? 1.0F : 0.0F;
        case Opcode::GREATER:
            return left > right ? 1.0F : 0.0F;
        case Opcode::GREATER_EQUAL:
            return left >= right ? 1.0F : 0.0F;
        case Opcode::EQUAL:
            return left == right ? 1.0F : 0.0F;
        case Opcode::NOT_EQUAL:
            return left != right ? 1.0F : 0.0F;
        case Opcode::AND:
            return left != 0.0F && right != 0.0F ? 1.0F : 0.0F;
        default:
            return left != 0.0F || right != 0.0F ? 1.0F : 0.0F;
    }
}

/**
 * Component index of a value, read as if the value had wide components. A float fills every
 * component, except that a float read as a matrix is the diagonal matrix. An assignment
 * promotes a float the same way.
 **/
float promoted(const Value & value, unsigned int point, unsigned int index, unsigned int wide) {
    if (value.components() != 1) {
        return value.component(point, index);
    }
    if (wide == 16 && index % MATRIX_DIAGONAL_STRIDE != 0) {
        return 0.0F;
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
        // a constant is written once here, not by an instruction on every run
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
    // a report made while a run is executing names the instruction's place in the source, so
    // one fault at two places is two reports
    const std::string placed = at_ != nullptr && at_->line > 0 ?
        "line " + std::to_string(at_->line) + ", column " + std::to_string(at_->column) + ": " + message : message;
    // once per distinct report, because every shading point of a render can make the same one
    if (std::ranges::find(reports_, placed) == reports_.end()) {
        reports_.push_back(placed);
        if (logger_) {
            logger_->get()->warn("shader: {}", placed);
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
    // live, not only while lane zero is. A lane that broke out of a loop must not stop a
    // uniform counter that other lanes are still advancing
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
            // a one-component operand applies to every component of the other, as RI promotes
            // a float. The missing components are not read as zero
            const float leftComponent = left.component(point, left.components() == 1 ? 0 : i);
            const float rightComponent = right.component(point, right.components() == 1 ? 0 : i);
            target.component(point, i, combine(instruction.opcode, leftComponent, rightComponent));
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
    const float same = left.text() == right.text() ? 1.0F : 0.0F;
    // two colours, points or matrices are equal when every component is. A float on one side
    // is promoted as an assignment promotes it, so a matrix compares against a diagonal
    const bool whole = (instruction.opcode == Opcode::EQUAL || instruction.opcode == Opcode::NOT_EQUAL) &&
        (left.components() > 1 || right.components() > 1);
    const unsigned int wide = std::max(left.components(), right.components());
    for (unsigned int point = 0; point < count; point++) {
        if (!writable(target, point)) {
            continue;
        }
        if (text) {
            target.number(point, instruction.opcode == Opcode::NOT_EQUAL ? 1.0F - same : same);
        } else if (whole) {
            bool equal = true;
            for (unsigned int i = 0; i < wide && equal; i++) {
                equal = promoted(left, point, i, wide) == promoted(right, point, i, wide);
            }
            target.number(point, equal == (instruction.opcode == Opcode::EQUAL) ? 1.0F : 0.0F);
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
            target.number(point, source.number(point) == 0.0F ? 1.0F : 0.0F);
            continue;
        }
        for (unsigned int i = 0; i < target.components(); i++) {
            target.component(point, i, -source.component(point, i));
        }
    }
}

glm::mat4x4 Machine::space(const std::string & name) {
    glm::mat4x4 matrix(1.0F);
    if (renderer_ == nullptr || !renderer_->space(name, &matrix)) {
        // the value still arrives, unchanged in the space it was already in
        report("the coordinate space \"" + name + "\" is not one this renderer knows");
    }
    return matrix;
}

bool Machine::projected(const std::string & name) const {
    return renderer_ != nullptr && renderer_->projected(name);
}

void Machine::transform(const Instruction & instruction) {
    Value & target = file_[static_cast<std::size_t>(instruction.target)];
    const Value & source = file_[static_cast<std::size_t>(instruction.left)];
    // only a point, vector or normal is in a coordinate space, so only those look the space up.
    // A cast's operand is in the named space and the shader works in the current space, so
    // the cast applies the inverse of the matrix the renderer returns
    const bool geometric = pointlike(target.type());
    const std::string & name = file_[static_cast<std::size_t>(instruction.right)].text();
    const glm::mat4x4 matrix = geometric ? glm::inverse(space(name)) : glm::mat4x4(1.0F);
    // a point stated in a projected space comes back to current space through the inverse
    // projection, and is right only after the divide
    const bool divided = geometric && projected(name);
    const unsigned int count = target.storage() == Storage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (!writable(target, point)) {
            continue;
        }
        switch (target.type()) {
            case Type::POINT:
                target.triple(point, divided ? project(matrix, source.triple(point)) :
                    ptransform(matrix, source.triple(point)));
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

void Machine::gather(const std::vector<glm::vec3> & axes, const std::vector<float> & angles) {
    gatherAxes_ = axes;
    gatherAngles_ = angles;
}

namespace {

/**
 * The direction inside a light's cone nearest a surface's axis: the axis itself when it lies
 * inside the cone, and otherwise the cone's edge on the way from the light's axis to it. It has
 * the length of the light's axis.
 **/
glm::vec3 nearest(const glm::vec3 & lightAxis, float lightAngle, const glm::vec3 & surfaceAxis) {
    const float length = glm::length(lightAxis);
    if (length == 0.0F || glm::length(surfaceAxis) == 0.0F) {
        return lightAxis;
    }
    const glm::vec3 from = lightAxis / length;
    const glm::vec3 to = glm::normalize(surfaceAxis);
    const float cosine = std::clamp(glm::dot(from, to), -1.0F, 1.0F);
    if (std::acos(cosine) <= lightAngle) {
        return to * length;
    }
    // the edge lies in the plane of the two axes, a light's angle away from its own axis
    constexpr float OPPOSITE = 1.0e-6F;
    glm::vec3 across = to - (from * cosine);
    if (glm::length(across) < OPPOSITE) {
        // the surface faces straight away from the light: any edge is as near as any other, so
        // the edge is taken toward whichever of x and y lies further from the axis
        const glm::vec3 other = std::fabs(from.x) < std::fabs(from.y) ? glm::vec3(1.0F, 0.0F, 0.0F) : glm::vec3(0.0F, 1.0F, 0.0F);
        across = glm::cross(from, other);
    }
    across = glm::normalize(across);
    return ((from * std::cos(lightAngle)) + (across * std::sin(lightAngle))) * length;
}

};  // namespace

bool Machine::inside(const glm::vec3 & direction, const std::vector<int> & cone,
    std::size_t first, unsigned int point) const {
    if (cone.size() < first + 2) {
        return true;
    }
    const glm::vec3 axis = file_[static_cast<std::size_t>(cone[first])].triple(point);
    const float angle = file_[static_cast<std::size_t>(cone[first + 1])].number(point);
    const float length = glm::length(direction) * glm::length(axis);
    if (length == 0.0F) {
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
        // the loop's own cone, which a solar light with an angle chooses its direction against
        Cone gathering;
        if (round.arguments.size() > 2) {
            gathering.axis = &file_[static_cast<std::size_t>(round.arguments[1])];
            gathering.angle = &file_[static_cast<std::size_t>(round.arguments[2])];
        }
        if (!renderer_->light(index, surface, gathering, &direction, &colour, &reached, &ambient) || ambient) {
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
            // the light, so the test uses L and not the direction the light travels
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
        glm::vec3 toward(0.0F);
        if (given.empty()) {
            // solar with no axis is light from every direction at once, which has no L
            direction.triple(point, toward);
        } else if (solar) {
            // the argument is the way the light travels, and L points back along it
            toward = -file_[static_cast<std::size_t>(given[0])].triple(point);
            // with an angle, L may be any direction inside a cone around that, and it is the one
            // nearest the axis of the cone the surface gathers from
            const float angle = given.size() > 1 ? file_[static_cast<std::size_t>(given[1])].number(point) : 0.0F;
            if (angle > 0.0F && point < gatherAxes_.size()) {
                toward = nearest(toward, angle, gatherAxes_[point]);
            }
            direction.triple(point, toward);
        } else {
            toward = file_[static_cast<std::size_t>(given[0])].triple(point) - surface.triple(point);
            direction.triple(point, toward);
        }
        // a light's own cone opens from the light outward, so the test uses the direction
        // the light travels and not L
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
        if (next[point] != 0 && (condition.number(point) != 0.0F) != wanted) {
            next[point] = 0;
        }
    }
    masks_.push_back(next);
}

void Machine::narrow(const Instruction & instruction) {
    Loop & loop = loops_.back();
    const Value & condition = file_[static_cast<std::size_t>(instruction.left)];
    for (unsigned int point = 0; point < batch_; point++) {
        if (loop.lanes[point] != 0 && condition.number(point) == 0.0F) {
            loop.lanes[point] = 0;
        }
    }
    masks_.back() = loop.lanes;
}

void Machine::finish() {
    // a return leaves only the function it is in, so the stacks are cleared from the depths
    // where that body opened, not from the bottom
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
    // a lane leaves by being cleared from every mask inside the loop. It cannot jump, because
    // the other lanes have not finished
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
    return execute(*program_, {.from = 0, .until = program_->prologue});
}

bool Machine::run() {
    if (program_ == nullptr) {
        error_ = "the machine was run before it was prepared";
        report(error_);
        return false;
    }
    if (!execute(*program_, {.from = program_->prologue, .until = program_->instructions.size()})) {
        // reported here, so a renderer does not log a failed run itself. initialise() does
        // not report, because Instance::write() reports its failure with the shader's name
        report(error_);
        return false;
    }
    return true;
}

bool Machine::execute(const Program & program, const Range & range) {
    const std::size_t from = range.from;
    const std::size_t until = range.until;
    error_.clear();
    printed_.clear();
    masks_.assign(1, std::vector<char>(batch_, 1));
    loops_.clear();
    frames_.clear();
    illuminations_.clear();
    // every point is lit until an illuminate or a solar narrows the set, so a light shader
    // with neither lights the whole batch
    lit_.assign(batch_, 1);

    std::size_t pc = from;
    std::size_t steps = 0;
    while (pc < until) {
        if (++steps > LIMIT) {
            error_ = "the shader '" + program.name + "' ran without end";
            at_ = nullptr;
            return false;
        }
        const Instruction & instruction = program.instructions[pc];
        at_ = &instruction;
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
                if (file_[static_cast<std::size_t>(instruction.left)].number(0) == 0.0F) {
                    pc = static_cast<std::size_t>(instruction.target);
                    continue;
                }
                break;
            case Opcode::MASK:
            case Opcode::MASK_NOT:
                mask(instruction, instruction.opcode == Opcode::MASK);
                // an arm that no lane took is skipped after the push
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
                // a lane that took a continue comes back for the next iteration. A lane that
                // took a break does not, because break cleared it from the loop's own lanes
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
                // the lanes that returned are live again, because they have finished the
                // function and not the shader
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
                // each narrows the batch to the points one light reaches, and skips the
                // body when the light reaches none of them
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
                // like a break, a return does not jump. It clears its lanes from every mask
                // and loop inside the function body. The rest of the body runs with those
                // lanes off, and LEAVE makes them live again
                finish();
                break;
        }
        pc++;
    }
    at_ = nullptr;
    return true;
}

};  // namespace v3d::render::offline::sl::runtime
