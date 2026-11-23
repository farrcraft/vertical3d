/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>

#include <moya/libmoya/colour/Transform.h>
#include <moya/libmoya/colour/internal/Configuration.h>
#include <moya/libmoya/colour/internal/Processor.h>
#include <OpenColorIO/OpenColorIO.h>

#include <stdexcept>
#include <string>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::moya::colour {

namespace {

namespace ocio = OCIO_NAMESPACE;

/** A processor's CPU form, at 32-bit floats, as a Transform. **/
Transform transformOf(const ocio::ConstProcessorRcPtr & processor) {
    return Transform(boost::make_shared<const internal::Processor>(processor->getDefaultCPUProcessor()));
}

};  // namespace

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

std::string Config::spaceOf(const std::string & interopId) const {
    const ocio::ConstConfigRcPtr & config = config_->get();
    for (int i = 0; i < config->getNumColorSpaces(ocio::SEARCH_REFERENCE_SPACE_ALL, ocio::COLORSPACE_ALL); ++i) {
        const char * name = config->getColorSpaceNameByIndex(ocio::SEARCH_REFERENCE_SPACE_ALL, ocio::COLORSPACE_ALL, i);
        const ocio::ConstColorSpaceRcPtr space = config->getColorSpace(name);
        if (space && interopId == space->getInteropID()) {
            return name;
        }
    }
    return {};
}

bool Config::data(const std::string & space) const {
    const ocio::ConstColorSpaceRcPtr found = config_->get()->getColorSpace(space.c_str());
    if (!found) {
        throw std::runtime_error("the colour configuration has no colour space " + space);
    }
    return found->isData();
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
