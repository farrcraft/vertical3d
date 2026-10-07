/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Globals.h"

#include <api/render/offline/sl/Types.h>

#include <vector>

namespace v3d::render::offline::sl {

namespace {

void put(runtime::Machine* machine, int reg, unsigned int lane, const glm::vec3 & value) {
    if (reg >= 0) {
        machine->value(reg).triple(lane, value);
    }
}

void put(runtime::Machine* machine, int reg, unsigned int lane, float value) {
    if (reg >= 0) {
        machine->value(reg).number(lane, value);
    }
}

};  // namespace

Globals::Globals(const runtime::Program & program) :
    position_(program.symbol("P")),
    normal_(program.symbol("N")),
    geometric_(program.symbol("Ng")),
    incident_(program.symbol("I")),
    eye_(program.symbol("E")),
    surfaceColour_(program.symbol("Cs")),
    surfaceOpacity_(program.symbol("Os")),
    colour_(program.symbol("Ci")),
    opacity_(program.symbol("Oi")),
    alpha_(program.symbol("alpha")),
    s_(program.symbol("s")),
    t_(program.symbol("t")),
    u_(program.symbol("u")),
    v_(program.symbol("v")),
    du_(program.symbol("du")),
    dv_(program.symbol("dv")),
    lit_(program.symbol("Ps")),
    towards_(program.symbol("L")),
    lightColour_(program.symbol("Cl")) {
}

void Globals::surface(runtime::Machine* machine, unsigned int lane, const Point & point) const {
    put(machine, position_, lane, point.position);
    put(machine, normal_, lane, point.normal);
    put(machine, geometric_, lane, point.geometric);
    put(machine, incident_, lane, point.incident);
    put(machine, surfaceColour_, lane, point.colour);
    put(machine, surfaceOpacity_, lane, point.opacity);
    put(machine, opacity_, lane, point.opacity);
    put(machine, s_, lane, point.s);
    put(machine, t_, lane, point.t);
    put(machine, u_, lane, point.u);
    put(machine, v_, lane, point.v);
    put(machine, du_, lane, point.du);
    put(machine, dv_, lane, point.dv);
}

void Globals::eye(runtime::Machine* machine, const glm::vec3 & position) const {
    put(machine, eye_, 0, position);
}

glm::vec3 Globals::colour(const runtime::Machine & machine, unsigned int lane, const glm::vec3 & otherwise) const {
    return colour_ < 0 ? otherwise : machine.value(colour_).triple(lane);
}

glm::vec3 Globals::opacity(const runtime::Machine & machine, unsigned int lane, const glm::vec3 & otherwise) const {
    return opacity_ < 0 ? otherwise : machine.value(opacity_).triple(lane);
}

void Globals::pixel(runtime::Machine* machine, unsigned int lane, const glm::vec3 & colour, float coverage,
    const glm::vec3 & position) const {
    put(machine, colour_, lane, colour);
    put(machine, opacity_, lane, glm::vec3(coverage));
    put(machine, alpha_, lane, coverage);
    put(machine, position_, lane, position);
}

float Globals::alpha(const runtime::Machine & machine, unsigned int lane, float otherwise) const {
    return alpha_ < 0 ? otherwise : machine.value(alpha_).number(lane);
}

bool Globals::shine(const Placed & light, runtime::Machine* machine, unsigned int batch, const runtime::Value & surface,
    runtime::Value* direction, runtime::Value* colour, std::vector<char>* reached, bool* ambient) const {
    if (!light.shader->write(machine, light.placement)) {
        return false;
    }

    const glm::vec3 origin = ptransform(light.placement, glm::vec3(0.0f));
    for (unsigned int lane = 0; lane < batch; lane++) {
        put(machine, lit_, lane, surface.triple(lane));
        put(machine, position_, lane, origin);
    }
    if (!machine->run()) {
        return false;
    }

    for (unsigned int lane = 0; lane < batch; lane++) {
        direction->triple(lane, towards_ < 0 ? glm::vec3(0.0f) : machine->value(towards_).triple(lane));
        colour->triple(lane, lightColour_ < 0 ? glm::vec3(0.0f) : machine->value(lightColour_).triple(lane));
    }
    *reached = machine->lit();
    *ambient = light.shader->ambient();
    return true;
}

};  // namespace v3d::render::offline::sl
