/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Instance.h"

#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::render::offline::sl {

namespace {

/**
 * The SL type a RIB declaration says the value is.
 *
 * RIB's integer is SL's float, because SL has no integer type; RIB's hpoint has no SL
 * reading at all and a parameter declared as one will not coerce.
 **/
Type declared(rib::Declaration::Type type) {
    switch (type) {
        case rib::Declaration::Type::FLOAT:
        case rib::Declaration::Type::INTEGER:
            return Type::FLOAT;
        case rib::Declaration::Type::STRING:
            return Type::STRING;
        case rib::Declaration::Type::COLOR:
            return Type::COLOR;
        case rib::Declaration::Type::POINT:
            return Type::POINT;
        case rib::Declaration::Type::VECTOR:
            return Type::VECTOR;
        case rib::Declaration::Type::NORMAL:
            return Type::NORMAL;
        case rib::Declaration::Type::MATRIX:
            return Type::MATRIX;
        case rib::Declaration::Type::HPOINT:
            break;
    }
    return Type::VOID;
}

/**
 * A position or a direction out of the space it was stated in and into the machine's.
 *
 * Which of the three transforms it takes is what tells the point-like types apart, and
 * getting it wrong is invisible under every uniform scale.
 **/
glm::vec3 moved(Type type, const glm::mat4x4 & placement, const glm::vec3 & given) {
    switch (type) {
        case Type::POINT:
            return ptransform(placement, given);
        case Type::VECTOR:
            return vtransform(placement, given);
        case Type::NORMAL:
            return ntransform(placement, given);
        default:
            // a colour has no space to be in, and a matrix is written component by
            // component below rather than through this
            return given;
    }
}

};  // namespace

Instance::Instance(const ProgramPtr & program, const boost::shared_ptr<v3d::log::Logger> & logger) :
    program_(program),
    logger_(logger) {
    // a light shader that never says where its light comes from lights every point of
    // every batch, which is what ambient() sums and an illuminance loop cannot reach
    ambient_ = program_->type == ShaderType::LIGHT &&
        std::ranges::none_of(program_->instructions, [](const runtime::Instruction & instruction) {
            return instruction.opcode == runtime::Opcode::ILLUMINATE ||
                instruction.opcode == runtime::Opcode::SOLAR;
        });
    for (std::size_t i = 0; i < program_->symbols; i++) {
        const runtime::Register & reg = program_->registers[i];
        if (!reg.parameter) {
            continue;
        }
        Binding held;
        held.name = reg.name;
        held.reg = static_cast<int>(i);
        held.type = reg.type;
        bindings_.push_back(held);
    }
}

const runtime::Program & Instance::program() const {
    return *program_;
}

const std::string & Instance::name() const {
    return program_->name;
}

ShaderType Instance::type() const {
    return program_->type;
}

bool Instance::ambient() const {
    return ambient_;
}

Instance::Binding* Instance::binding(const std::string & wanted) {
    for (Binding & held : bindings_) {
        if (held.name == wanted) {
            return &held;
        }
    }
    return nullptr;
}

void Instance::bind(const rib::ParameterList & parameters) {
    for (const std::string & given : parameters.names()) {
        Binding* held = binding(given);
        if (held == nullptr) {
            // a renderer must accept a request carrying a parameter it does not support
            logger_->get()->warn("the shader '{}' declares no '{}', which was dropped",
                program_->name, given);
            continue;
        }
        const rib::Declaration* declaration = parameters.declaration(given);
        const Type type = declaration == nullptr ? Type::VOID : declared(declaration->type());
        if (!coercible(type, held->type)) {
            // reinterpreted rather than reported is how a picture comes out wrong quietly
            logger_->get()->warn("'{}' of the shader '{}' is {} and the scene bound {}",
                given, program_->name, sl::name(held->type),
                type == Type::VOID ? "something with no reading" : sl::name(type));
            continue;
        }
        held->bound = true;
        if (held->type == Type::STRING) {
            held->text = parameters.string(given, std::string());
            continue;
        }
        const std::vector<float> & values = parameters.floats(given);
        held->values.assign(components(held->type), 0.0f);
        for (unsigned int component = 0; component < held->values.size(); component++) {
            // a float bound onto a colour replicates, which is RI's promotion and is what
            // "Color [1]" and a one value "specularcolor" both mean
            const std::size_t which = values.size() == 1 ? 0 : component;
            if (which < values.size()) {
                held->values[component] = values[which];
            }
        }
    }
}

void Instance::write(runtime::Machine* machine, const glm::mat4x4 & placement) const {
    // the declared defaults, run rather than remembered: what a coordinate space in one
    // comes to is the renderer's answer, and the machine has one now
    machine->initialise(*program_);

    for (const Binding & held : bindings_) {
        if (!held.bound) {
            continue;
        }
        runtime::Value & value = machine->value(held.reg);
        if (held.type == Type::STRING) {
            value.text(held.text);
            continue;
        }
        // a position or a direction a scene binds is stated in the space that was in
        // force when it instanced the shader, and the machine works in the current one
        const bool triple = held.values.size() == 3;
        const glm::vec3 place = triple ?
            moved(held.type, placement, glm::vec3(held.values[0], held.values[1], held.values[2])) :
            glm::vec3(0.0f);
        // a uniform parameter is one element and a varying one is the batch, and a scene
        // binds one value either way: every point of the batch gets it
        for (unsigned int point = 0; point < value.width(); point++) {
            for (unsigned int component = 0; component < held.values.size(); component++) {
                value.component(point, component,
                    triple ? place[component] : held.values[component]);
            }
        }
    }
}

};  // namespace v3d::render::offline::sl
