/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "Container.h"
#include "Loader.h"
#include "Style.h"
#include "component/Button.h"
#include "component/Icon.h"
#include "component/Toolbar.h"
#include "component/Type.h"
#include "style/Theme.h"
#include "style/property/Image.h"

#include "../asset/Json.h"
#include "../log/Logger.h"

#include <boost/make_shared.hpp>

namespace v3d::ui {

Engine::Engine(const boost::shared_ptr<v3d::event::Engine>& eventEngine, const boost::shared_ptr<entt::dispatcher>& dispatcher,
    const boost::shared_ptr<v3d::log::Logger>& logger) :
    eventEngine_(eventEngine), dispatcher_(dispatcher), logger_(logger) {
}

bool Engine::load(const boost::shared_ptr<v3d::asset::Json>& config) {
    Loader loader(eventEngine_, dispatcher_, logger_);
    if (!loader.load(config->document())) {
        return false;
    }
    containers_ = std::move(loader.containers());
    themes_ = std::move(loader.themes());

    // the first theme loaded is active unless the document named one, which is what makes
    // a config carrying a single theme need no field at all
    activeTheme_ = themes_.empty() ? nullptr : themes_.front();
    if (!loader.active().empty()) {
        activeTheme(loader.active());
    }
    return true;
}

/**
 **/
std::size_t Engine::resolveThemeImages(const Resolve& resolve) {
    std::size_t resolved = 0;
    for (const boost::shared_ptr<style::Theme>& theme : themes_) {
        for (const boost::shared_ptr<Style>& target : theme->getStyleSet("", "")) {
            for (const boost::shared_ptr<style::Property>& property : target->getPropertySet("", "image")) {
                boost::shared_ptr<style::property::Image> image =
                    boost::dynamic_pointer_cast<style::property::Image>(property);
                if (!image) {
                    continue;
                }
                const v3d::render::realtime::TextureHandle texture = resolve(std::string(image->source()));
                if (!texture.valid()) {
                    logger_->get()->error("Unable to resolve the ui image [{}]", image->source());
                    continue;
                }
                image->texture(texture);
                resolved++;
            }
        }
    }
    return resolved;
}

std::size_t Engine::resolveComponentImages(const Resolve& resolve, const boost::shared_ptr<Component>& component) {
    std::size_t resolved = 0;

    boost::shared_ptr<component::Icon> icon = boost::dynamic_pointer_cast<component::Icon>(component);
    if (icon) {
        if (resolveIcon(resolve, std::string(icon->source()), icon)) {
            resolved++;
        }
        return resolved;
    }
    boost::shared_ptr<component::Button> button = boost::dynamic_pointer_cast<component::Button>(component);
    if (button) {
        if (resolveIcon(resolve, std::string(button->icon()), button)) {
            resolved++;
        }
        return resolved;
    }
    // a strip's buttons are its own rather than the container's, so they are not
    // reached by walking what the container holds
    boost::shared_ptr<component::Toolbar> bar = boost::dynamic_pointer_cast<component::Toolbar>(component);
    if (!bar) {
        return resolved;
    }
    for (std::size_t index = 0; index < bar->size(); index++) {
        const boost::shared_ptr<component::Button> held = bar->button(index);
        if (held && resolveIcon(resolve, std::string(held->icon()), held)) {
            resolved++;
        }
    }
    return resolved;
}

std::size_t Engine::resolveContainerImages(const Resolve& resolve) {
    std::size_t resolved = 0;
    for (const boost::shared_ptr<Container>& container : containers_) {
        for (const boost::shared_ptr<Component>& component : container->components()) {
            resolved += resolveComponentImages(resolve, component);
        }
    }
    return resolved;
}

std::size_t Engine::resolveImages(const Resolve& resolve) {
    if (!resolve) {
        return 0;
    }
    return resolveThemeImages(resolve) + resolveContainerImages(resolve);
}

/**
 **/
template <typename T>
bool Engine::resolveIcon(const Resolve& resolve, const std::string& source, const boost::shared_ptr<T>& target) {
    // a component naming no image is not a failure - most of them name none
    if (source.empty()) {
        return false;
    }
    const v3d::render::realtime::TextureHandle texture = resolve(source);
    if (!texture.valid()) {
        logger_->get()->error("Unable to resolve the ui image [{}]", source);
        return false;
    }
    target->texture(texture);
    return true;
}

/**
 **/
boost::shared_ptr<style::Theme> Engine::theme(const std::string_view& name) const {
    auto it = themes_.begin();
    for (; it != themes_.end(); ++it) {
        if ((*it)->name() == name) {
            return *it;
        }
    }
    return nullptr;
}

/**
 **/
boost::shared_ptr<style::Theme> Engine::activeTheme() const {
    return activeTheme_;
}

/**
 **/
bool Engine::activeTheme(const std::string_view& name) {
    boost::shared_ptr<style::Theme> found = theme(name);
    if (!found) {
        logger_->get()->error("No such ui theme [{}]", name);
        return false;
    }
    activeTheme_ = found;
    return true;
}

boost::shared_ptr<Container> Engine::container(const std::string_view& name) {
    auto containers = containers_.begin();
    for (; containers != containers_.end(); ++containers) {
        if ((*containers)->name() == name) {
            return *containers;
        }
    }
    return nullptr;
}
/**
 **/
const std::vector<boost::shared_ptr<Container>>& Engine::containers() const noexcept {
    return containers_;
}

};  // namespace v3d::ui
