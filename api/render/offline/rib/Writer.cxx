/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Writer.h"

#include <api/render/offline/Sampling.h>

#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <system_error>
#include <ostream>
#include <string>
#include <vector>

#include "Declaration.h"
#include "Declarations.h"
#include "Handler.h"
#include "Parameters.h"
#include "Requests.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::rib {

Writer::Writer(std::ostream * stream) : stream_(stream) {
}

Writer & Writer::start(const char * name) {
    *stream_ << name;
    return *this;
}

Writer & Writer::number(float value) {
    if (std::isnan(value)) {
        value = 0.0F;
    } else if (std::isinf(value)) {
        value = std::copysign(std::numeric_limits<float>::max(), value);
    }
    // the shortest form that reads back as the same float
    std::array<char, 32> digits {};
    const std::to_chars_result written = std::to_chars(digits.data(), digits.data() + digits.size(), value);
    *stream_ << ' ' << std::string(digits.data(), written.ptr);
    return *this;
}

Writer & Writer::text(const std::string & value) {
    *stream_ << " \"";
    for (const char character : value) {
        if (character == '"' || character == '\\') {
            *stream_ << '\\' << character;
        } else if (character == '\n') {
            *stream_ << "\\n";
        } else {
            *stream_ << character;
        }
    }
    *stream_ << '"';
    return *this;
}

Writer & Writer::array(const std::vector<float> & values) {
    *stream_ << " [";
    for (const float value : values) {
        number(value);
    }
    *stream_ << " ]";
    return *this;
}

Writer & Writer::counts(const std::vector<unsigned int> & values) {
    *stream_ << " [";
    for (const unsigned int value : values) {
        *stream_ << ' ' << value;
    }
    *stream_ << " ]";
    return *this;
}

Writer & Writer::handle(const std::string & value) {
    // a number only when it is one exactly as written, and within the range a reader takes
    constexpr int32_t LARGEST = 2000000000;
    int32_t parsed = 0;
    const std::from_chars_result read = std::from_chars(value.data(), value.data() + value.size(), parsed);
    const bool whole = read.ec == std::errc() && read.ptr == value.data() + value.size();
    if (whole && parsed >= -LARGEST && parsed <= LARGEST && std::to_string(parsed) == value) {
        *stream_ << ' ' << value;
        return *this;
    }
    return text(value);
}

Writer & Writer::parameters(const ParameterList & list) {
    for (const std::string & name : list.names()) {
        const Declaration & declaration = *list.declaration(name);
        std::string known;
        Declaration inForce;
        const bool declared = declarations_.resolve(name, &known, &inForce) && inForce == declaration;
        text(declared ? name : declaration.text() + " " + name);
        if (declaration.type() == Declaration::Type::STRING) {
            *stream_ << " [";
            for (const std::string & value : list.strings(name)) {
                text(value);
            }
            *stream_ << " ]";
        } else {
            array(list.floats(name));
        }
    }
    return *this;
}

void Writer::end() {
    *stream_ << '\n';
}

void Writer::version(float number) {
    start("version").number(number).end();
}

void Writer::declare(const Declare & request) {
    // a declaration that does not parse declares nothing here, as it declares nothing in a reader
    (void)declarations_.declare(request);
    start("Declare").text(request.name).text(request.type).end();
}

void Writer::option(const std::string & name, const ParameterList & parameters) {
    start("Option").text(name).parameters(parameters).end();
}

void Writer::hider(const std::string & name, const ParameterList & parameters) {
    start("Hider").text(name).parameters(parameters).end();
}

void Writer::format(const Format & request) {
    *stream_ << "Format " << request.width << ' ' << request.height;
    number(request.pixelAspect).end();
}

void Writer::frameAspectRatio(float aspect) {
    start("FrameAspectRatio").number(aspect).end();
}

void Writer::screenWindow(const ScreenWindow & request) {
    start("ScreenWindow").number(request.left).number(request.right).number(request.bottom).number(request.top).end();
}

void Writer::cropWindow(const CropWindow & request) {
    start("CropWindow").number(request.xmin).number(request.xmax).number(request.ymin).number(request.ymax).end();
}

void Writer::projection(const std::string & name, const ParameterList & parameters) {
    start("Projection").text(name).parameters(parameters).end();
}

void Writer::clipping(const Clipping & request) {
    start("Clipping").number(request.hither).number(request.yon).end();
}

void Writer::depthOfField(const DepthOfField & request) {
    start("DepthOfField");
    if (!std::isinf(request.fstop)) {
        number(request.fstop).number(request.focalLength).number(request.focalDistance);
    }
    end();
}

void Writer::shutter(const Shutter & request) {
    start("Shutter").number(request.open).number(request.close).end();
}

void Writer::pixelSamples(const glm::uvec2 & samples) {
    *stream_ << "PixelSamples " << samples.x << ' ' << samples.y;
    end();
}

void Writer::pixelFilter(Filter filter, float xwidth, float ywidth) {
    start("PixelFilter").text(filterName(filter)).number(xwidth).number(ywidth).end();
}

void Writer::pixelVariance(float variation) {
    start("PixelVariance").number(variation).end();
}

void Writer::display(const Display & request, const ParameterList & parameters) {
    start("Display").text(request.name).text(request.type).text(request.mode).parameters(parameters).end();
}

void Writer::frameBegin(int frame) {
    *stream_ << "FrameBegin " << frame;
    end();
}

void Writer::frameEnd() {
    start("FrameEnd").end();
}

void Writer::worldBegin() {
    start("WorldBegin").end();
}

void Writer::worldEnd() {
    start("WorldEnd").end();
}

void Writer::attributeBegin() {
    start("AttributeBegin").end();
}

void Writer::attributeEnd() {
    start("AttributeEnd").end();
}

void Writer::transformBegin() {
    start("TransformBegin").end();
}

void Writer::transformEnd() {
    start("TransformEnd").end();
}

void Writer::identity() {
    start("Identity").end();
}

// the sixteen floats in the order glm stores them, which is RIB's order, as ParameterList::matrix
// explains
void Writer::transform(const glm::mat4x4 & matrix) {
    start("Transform").array(std::vector<float>(&matrix[0][0], &matrix[0][0] + 16)).end();
}

void Writer::concatTransform(const glm::mat4x4 & matrix) {
    start("ConcatTransform").array(std::vector<float>(&matrix[0][0], &matrix[0][0] + 16)).end();
}

void Writer::translate(float dx, float dy, float dz) {
    start("Translate").number(dx).number(dy).number(dz).end();
}

void Writer::rotate(float angle, const glm::vec3 & axis) {
    start("Rotate").number(angle).number(axis.x).number(axis.y).number(axis.z).end();
}

void Writer::scale(float sx, float sy, float sz) {
    start("Scale").number(sx).number(sy).number(sz).end();
}

void Writer::coordinateSystem(const std::string & space) {
    start("CoordinateSystem").text(space).end();
}

void Writer::coordSysTransform(const std::string & space) {
    start("CoordSysTransform").text(space).end();
}

void Writer::motionBegin(const std::vector<float> & times) {
    start("MotionBegin").array(times).end();
}

void Writer::motionEnd() {
    start("MotionEnd").end();
}

void Writer::color(const glm::vec3 & value) {
    start("Color").array({ value.x, value.y, value.z }).end();
}

void Writer::opacity(const glm::vec3 & value) {
    start("Opacity").array({ value.x, value.y, value.z }).end();
}

void Writer::shadingRate(float size) {
    start("ShadingRate").number(size).end();
}

void Writer::attribute(const std::string & name, const ParameterList & parameters) {
    start("Attribute").text(name).parameters(parameters).end();
}

void Writer::surface(const std::string & name, const ParameterList & parameters) {
    start("Surface").text(name).parameters(parameters).end();
}

void Writer::displacement(const std::string & name, const ParameterList & parameters) {
    start("Displacement").text(name).parameters(parameters).end();
}

void Writer::lightSource(const LightSource & request, const ParameterList & parameters) {
    start("LightSource").text(request.shader).handle(request.handle).parameters(parameters).end();
}

void Writer::areaLightSource(const LightSource & request, const ParameterList & parameters) {
    start("AreaLightSource").text(request.shader).handle(request.handle).parameters(parameters).end();
}

void Writer::illuminate(const std::string & handle, bool on) {
    start("Illuminate").handle(handle);
    *stream_ << (on ? " 1" : " 0");
    end();
}

void Writer::makeTexture(const MakeTexture & request, const ParameterList & parameters) {
    start("MakeTexture").text(request.picture).text(request.texture).text(request.swrap).text(request.twrap)
        .text(request.filter).number(request.swidth).number(request.twidth).parameters(parameters).end();
}

void Writer::imager(const std::string & name, const ParameterList & parameters) {
    start("Imager").text(name).parameters(parameters).end();
}

Handler* Writer::deformation() {
    return this;
}

// RIB carries no vertex count: a reader takes it from the length of "P"
void Writer::polygon(unsigned int vertices, const ParameterList & parameters) {
    (void)vertices;
    start("Polygon").parameters(parameters).end();
}

void Writer::pointsPolygons(const PointsPolygons & request, const ParameterList & parameters) {
    start("PointsPolygons").counts(request.counts).counts(request.indices).parameters(parameters).end();
}

void Writer::sphere(const Sphere & request, const ParameterList & parameters) {
    start("Sphere").number(request.radius).number(request.zmin).number(request.zmax).number(request.thetamax)
        .parameters(parameters).end();
}

};  // namespace v3d::render::offline::rib
