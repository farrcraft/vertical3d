/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Instance.h"

#include <string>
#include <vector>

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
    defaults();
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

void Instance::defaults() {
    /*
        The prologue is a run of its own over a batch of one. A default is one value
        whatever storage the parameter has: there is no shading point yet for it to differ
        at, and a scene binds one value per parameter in any case.
    */
    runtime::Machine machine;
    machine.prepare(*program_, 1);
    machine.initialise(*program_);
    for (const std::string & report : machine.reports()) {
        // a default written as "point \"shader\" (0, 0, 1)" is one of these: what a
        // shader's own space is has no answer until a renderer has placed the instance
        logger_->get()->warn("the defaults of the shader '{}' - {}", program_->name, report);
    }

    for (std::size_t i = 0; i < program_->symbols; i++) {
        const runtime::Register & reg = program_->registers[i];
        if (!reg.parameter) {
            continue;
        }
        Binding held;
        held.name = reg.name;
        held.reg = static_cast<int>(i);
        held.type = reg.type;
        held.text = machine.value(held.reg).text();
        for (unsigned int component = 0; component < components(reg.type); component++) {
            held.values.push_back(machine.value(held.reg).component(0, component));
        }
        bindings_.push_back(held);
    }
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
        if (held->type == Type::STRING) {
            held->text = parameters.string(given, held->text);
            continue;
        }
        const std::vector<float> & values = parameters.floats(given);
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

void Instance::write(runtime::Machine* machine) const {
    for (const Binding & held : bindings_) {
        runtime::Value & value = machine->value(held.reg);
        value.text(held.text);
        // a uniform parameter is one element and a varying one is the batch, and a scene
        // binds one value either way: every point of the batch gets it
        for (unsigned int point = 0; point < value.width(); point++) {
            for (unsigned int component = 0; component < held.values.size(); component++) {
                value.component(point, component, held.values[component]);
            }
        }
    }
}

};  // namespace v3d::render::offline::sl
