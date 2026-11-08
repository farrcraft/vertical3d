/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>

#include <moya/libmoya/colour/internal/Configuration.h>
#include <moya/libmoya/colour/internal/Processor.h>
#include <OpenColorIO/OpenColorIO.h>

#include <cstddef>
#include <string>
#include <utility>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

namespace v3d::moya::colour {

namespace {

namespace ocio = OCIO_NAMESPACE;

/** The name ACEScg has in OpenColorIO's built-in configurations. **/
constexpr const char * kAcescg = "ACEScg";

/** A processor's CPU form, at 32-bit floats, as a Transform. **/
Transform transformOf(const ocio::ConstProcessorRcPtr & processor) {
    return Transform(boost::make_shared<const internal::Processor>(processor->getDefaultCPUProcessor()));
}

};  // namespace

Transform::Transform(boost::shared_ptr<const internal::Processor> processor) : processor_(std::move(processor)) {
}

glm::vec3 Transform::apply(const glm::vec3 & rgb) const {
    float pixel[3] = { rgb.r, rgb.g, rgb.b };
    processor_->cpu()->applyRGB(pixel);
    return { pixel[0], pixel[1], pixel[2] };
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the count, then the stride
void Transform::apply(float * values, std::size_t pixels, std::size_t stride) const {
    if (pixels == 0) {
        return;
    }
    const auto bytes = static_cast<std::ptrdiff_t>(stride * sizeof(float));
    // OpenColorIO takes the image's width as a long
    const auto width = static_cast<long>(pixels);  // NOLINT(runtime/int)
    const ocio::PackedImageDesc image(values, width, 1, 3, ocio::BIT_DEPTH_F32, sizeof(float), bytes, bytes * static_cast<std::ptrdiff_t>(pixels));
    processor_->cpu()->apply(image);
}

Config::Config() : Config(std::string(kBuiltIn)) {
}

Config::Config(const std::string & path) {
    // OpenColorIO reads an ocio:// name as one of its built-in configurations
    const ocio::ConstConfigRcPtr config = ocio::Config::CreateFromFile(path.c_str());
    config->validate();
    config_ = boost::make_shared<const internal::Configuration>(config);
}

std::string Config::name() const {
    return config_->get()->getName();
}

std::string Config::workingSpace() const {
    const ocio::ConstColorSpaceRcPtr space = config_->get()->getColorSpace(ocio::ROLE_SCENE_LINEAR);
    return space ? space->getName() : std::string();
}

std::string Config::workingInteropId() const {
    const ocio::ConstColorSpaceRcPtr space = config_->get()->getColorSpace(ocio::ROLE_SCENE_LINEAR);
    return space ? space->getInteropID() : std::string();
}

Transform Config::input(const std::string & space) const {
    const std::string from = space.empty() ? workingSpace() : space;
    return transformOf(ocio::Config::GetProcessorToBuiltinColorSpace(config_->get(), from.c_str(), kAcescg));
}

Transform Config::output() const {
    return transformOf(ocio::Config::GetProcessorFromBuiltinColorSpace(kAcescg, config_->get(), workingSpace().c_str()));
}

std::string Config::defaultDisplay() const {
    return config_->get()->getDefaultDisplay();
}

std::string Config::defaultView(const std::string & display) const {
    return config_->get()->getDefaultView(display.c_str());
}

Transform Config::display(const std::string & display, const std::string & view) const {
    // ACEScg into the working space, then the working space through the display's view
    const ocio::ConstProcessorRcPtr toWorking =
        ocio::Config::GetProcessorFromBuiltinColorSpace(kAcescg, config_->get(), workingSpace().c_str());
    const ocio::GroupTransformRcPtr group = ocio::GroupTransform::Create();
    group->appendTransform(toWorking->createGroupTransform());
    const ocio::DisplayViewTransformRcPtr shown = ocio::DisplayViewTransform::Create();
    shown->setSrc(workingSpace().c_str());
    shown->setDisplay(display.c_str());
    shown->setView(view.c_str());
    group->appendTransform(shown);
    return transformOf(config_->get()->getProcessor(group));
}

};  // namespace v3d::moya::colour
