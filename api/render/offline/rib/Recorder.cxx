/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Recorder.h"

#include <api/render/offline/Sampling.h>

#include <string>
#include <vector>

#include "Handler.h"
#include "Parameters.h"
#include "Requests.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::rib {

void Recorder::replay(Handler * handler) const {
    for (const Request & request : requests_) {
        request(*handler);
    }
}

bool Recorder::empty() const {
    return requests_.empty();
}

void Recorder::version(float number) {
    requests_.emplace_back([number](Handler & to) { to.version(number); });
}

void Recorder::declare(const Declare & request) {
    requests_.emplace_back([request](Handler & to) { to.declare(request); });
}

void Recorder::option(const std::string & name, const ParameterList & parameters) {
    requests_.emplace_back([name, parameters](Handler & to) { to.option(name, parameters); });
}

void Recorder::hider(const std::string & name, const ParameterList & parameters) {
    requests_.emplace_back([name, parameters](Handler & to) { to.hider(name, parameters); });
}

void Recorder::format(const Format & request) {
    requests_.emplace_back([request](Handler & to) { to.format(request); });
}

void Recorder::frameAspectRatio(float aspect) {
    requests_.emplace_back([aspect](Handler & to) { to.frameAspectRatio(aspect); });
}

void Recorder::screenWindow(const ScreenWindow & request) {
    requests_.emplace_back([request](Handler & to) { to.screenWindow(request); });
}

void Recorder::cropWindow(const CropWindow & request) {
    requests_.emplace_back([request](Handler & to) { to.cropWindow(request); });
}

void Recorder::projection(const std::string & name, const ParameterList & parameters) {
    requests_.emplace_back([name, parameters](Handler & to) { to.projection(name, parameters); });
}

void Recorder::clipping(const Clipping & request) {
    requests_.emplace_back([request](Handler & to) { to.clipping(request); });
}

void Recorder::depthOfField(const DepthOfField & request) {
    requests_.emplace_back([request](Handler & to) { to.depthOfField(request); });
}

void Recorder::shutter(const Shutter & request) {
    requests_.emplace_back([request](Handler & to) { to.shutter(request); });
}

void Recorder::pixelSamples(const glm::uvec2 & samples) {
    requests_.emplace_back([samples](Handler & to) { to.pixelSamples(samples); });
}

void Recorder::pixelFilter(Filter filter, float xwidth, float ywidth) {
    requests_.emplace_back([filter, xwidth, ywidth](Handler & to) { to.pixelFilter(filter, xwidth, ywidth); });
}

void Recorder::pixelVariance(float variation) {
    requests_.emplace_back([variation](Handler & to) { to.pixelVariance(variation); });
}

void Recorder::display(const Display & request, const ParameterList & parameters) {
    requests_.emplace_back([request, parameters](Handler & to) { to.display(request, parameters); });
}

void Recorder::frameBegin(int frame) {
    requests_.emplace_back([frame](Handler & to) { to.frameBegin(frame); });
}

void Recorder::frameEnd() {
    requests_.emplace_back([](Handler & to) { to.frameEnd(); });
}

void Recorder::worldBegin() {
    requests_.emplace_back([](Handler & to) { to.worldBegin(); });
}

void Recorder::worldEnd() {
    requests_.emplace_back([](Handler & to) { to.worldEnd(); });
}

void Recorder::attributeBegin() {
    requests_.emplace_back([](Handler & to) { to.attributeBegin(); });
}

void Recorder::attributeEnd() {
    requests_.emplace_back([](Handler & to) { to.attributeEnd(); });
}

void Recorder::transformBegin() {
    requests_.emplace_back([](Handler & to) { to.transformBegin(); });
}

void Recorder::transformEnd() {
    requests_.emplace_back([](Handler & to) { to.transformEnd(); });
}

void Recorder::identity() {
    requests_.emplace_back([](Handler & to) { to.identity(); });
}

void Recorder::transform(const glm::mat4x4 & matrix) {
    requests_.emplace_back([matrix](Handler & to) { to.transform(matrix); });  // NOLINT(build/include_what_you_use)
}

void Recorder::concatTransform(const glm::mat4x4 & matrix) {
    requests_.emplace_back([matrix](Handler & to) { to.concatTransform(matrix); });
}

void Recorder::translate(float dx, float dy, float dz) {
    requests_.emplace_back([dx, dy, dz](Handler & to) { to.translate(dx, dy, dz); });
}

void Recorder::rotate(float angle, const glm::vec3 & axis) {
    requests_.emplace_back([angle, axis](Handler & to) { to.rotate(angle, axis); });
}

void Recorder::scale(float sx, float sy, float sz) {
    requests_.emplace_back([sx, sy, sz](Handler & to) { to.scale(sx, sy, sz); });
}

void Recorder::coordinateSystem(const std::string & space) {
    requests_.emplace_back([space](Handler & to) { to.coordinateSystem(space); });
}

void Recorder::coordSysTransform(const std::string & space) {
    requests_.emplace_back([space](Handler & to) { to.coordSysTransform(space); });
}

void Recorder::motionBegin(const std::vector<float> & times) {
    requests_.emplace_back([times](Handler & to) { to.motionBegin(times); });
}

void Recorder::motionEnd() {
    requests_.emplace_back([](Handler & to) { to.motionEnd(); });
}

void Recorder::basis(const Basis & request) {
    requests_.emplace_back([request](Handler & to) { to.basis(request); });
}

void Recorder::color(const glm::vec3 & value) {
    requests_.emplace_back([value](Handler & to) { to.color(value); });
}

void Recorder::opacity(const glm::vec3 & value) {
    requests_.emplace_back([value](Handler & to) { to.opacity(value); });
}

void Recorder::shadingRate(float size) {
    requests_.emplace_back([size](Handler & to) { to.shadingRate(size); });
}

void Recorder::attribute(const std::string & name, const ParameterList & parameters) {
    requests_.emplace_back([name, parameters](Handler & to) { to.attribute(name, parameters); });
}

void Recorder::surface(const std::string & name, const ParameterList & parameters) {
    requests_.emplace_back([name, parameters](Handler & to) { to.surface(name, parameters); });
}

void Recorder::displacement(const std::string & name, const ParameterList & parameters) {
    requests_.emplace_back([name, parameters](Handler & to) { to.displacement(name, parameters); });
}

void Recorder::lightSource(const LightSource & request, const ParameterList & parameters) {
    requests_.emplace_back([request, parameters](Handler & to) { to.lightSource(request, parameters); });
}

void Recorder::areaLightSource(const LightSource & request, const ParameterList & parameters) {
    requests_.emplace_back([request, parameters](Handler & to) { to.areaLightSource(request, parameters); });
}

void Recorder::illuminate(const std::string & handle, bool on) {
    requests_.emplace_back([handle, on](Handler & to) { to.illuminate(handle, on); });
}

void Recorder::makeTexture(const MakeTexture & request, const ParameterList & parameters) {
    requests_.emplace_back([request, parameters](Handler & to) { to.makeTexture(request, parameters); });
}

void Recorder::imager(const std::string & name, const ParameterList & parameters) {
    requests_.emplace_back([name, parameters](Handler & to) { to.imager(name, parameters); });
}

Handler* Recorder::deformation() {
    return &deforming_;
}

void Recorder::polygon(unsigned int vertices, const ParameterList & parameters) {
    requests_.emplace_back([vertices, parameters](Handler & to) { to.polygon(vertices, parameters); });
}

void Recorder::pointsPolygons(const PointsPolygons & request, const ParameterList & parameters) {
    requests_.emplace_back([request, parameters](Handler & to) { to.pointsPolygons(request, parameters); });
}

void Recorder::sphere(const Sphere & request, const ParameterList & parameters) {
    requests_.emplace_back([request, parameters](Handler & to) { to.sphere(request, parameters); });
}

void Recorder::curves(const Curves & request, const ParameterList & parameters) {
    requests_.emplace_back([request, parameters](Handler & to) { to.curves(request, parameters); });
}

Recorder::Deforming::Deforming(Recorder * recording) : recording_(recording) {
}

// a handler with no deformation() drops the primitive, as a reader does
void Recorder::Deforming::keep(const Request & primitive) {
    recording_->requests_.emplace_back([primitive](Handler & to) {
        Handler* deformation = to.deformation();
        if (deformation != nullptr) {
            primitive(*deformation);
        }
    });
}

void Recorder::Deforming::polygon(unsigned int vertices, const ParameterList & parameters) {
    keep([vertices, parameters](Handler & to) { to.polygon(vertices, parameters); });
}

void Recorder::Deforming::pointsPolygons(const PointsPolygons & request, const ParameterList & parameters) {
    keep([request, parameters](Handler & to) { to.pointsPolygons(request, parameters); });
}

void Recorder::Deforming::sphere(const Sphere & request, const ParameterList & parameters) {
    keep([request, parameters](Handler & to) { to.sphere(request, parameters); });
}

void Recorder::Deforming::curves(const Curves & request, const ParameterList & parameters) {
    keep([request, parameters](Handler & to) { to.curves(request, parameters); });
}

};  // namespace v3d::render::offline::rib
