/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Engine.h"

#include <api/asset/kind/Json.h>
#include <api/log/Logger.h>
#include <api/ui/component/Box.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Icon.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/Type.h>
#include <api/ui/style/Style.h>
#include <api/ui/style/Theme.h>
#include <api/ui/style/property/Image.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "Component.h"
#include "Container.h"
#include "Loader.h"

#include <boost/make_shared.hpp>

namespace v3d::ui {

Engine::Engine(const boost::shared_ptr<v3d::event::Engine>& eventEngine, const boost::shared_ptr<entt::dispatcher>& dispatcher,
    const boost::shared_ptr<v3d::log::Logger>& logger) :
    eventEngine_(eventEngine), dispatcher_(dispatcher), logger_(logger) {
}

bool Engine::load(const boost::shared_ptr<v3d::asset::kind::Json>& config) {
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
        for (const boost::shared_ptr<style::Style>& target : theme->getStyleSet("", "")) {
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
    boost::shared_ptr<component::Button> button = boost::dynamic_pointer_cast<component::Button>(component);
    // a strip's buttons are its own rather than children, so they are not reached by the
    // walk below and are taken here
    boost::shared_ptr<component::Toolbar> bar = boost::dynamic_pointer_cast<component::Toolbar>(component);
    if (icon) {
        if (resolveIcon(resolve, std::string(icon->source()), icon)) {
            resolved++;
        }
    } else if (button) {
        if (resolveIcon(resolve, std::string(button->icon()), button)) {
            resolved++;
        }
    } else if (bar) {
        for (std::size_t index = 0; index < bar->count(); index++) {
            const boost::shared_ptr<component::Button> held = bar->button(index);
            if (held && resolveIcon(resolve, std::string(held->icon()), held)) {
                resolved++;
            }
        }
    }

    // a container holds only what was added to it, and Loader gives a nested component to its
    // parent rather than to the container, so an icon inside a panel or a box is reached from
    // here and from nowhere else
    for (const boost::shared_ptr<Component>& child : component->children()) {
        resolved += resolveComponentImages(resolve, child);
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
void Engine::focus(const boost::shared_ptr<Component>& component) {
    // a component that cannot be used is nothing to focus, the same answer one that never
    // asked to be focusable gets - ADR-0059
    const boost::shared_ptr<Component> wanted =
        component && component->focusable() && usable(*component) ? component : boost::shared_ptr<Component>();
    const boost::shared_ptr<Component> was = focused_.lock();
    if (was == wanted) {
        return;
    }
    if (was) {
        was->focused(false);
    }
    if (wanted) {
        wanted->focused(true);
    }
    focused_ = wanted;
    // after both components have been told, so that a listener asking focused() is answered
    // the move rather than the middle of it
    if (moved_) {
        moved_(wanted);
    }
}

/**
 **/
void Engine::onFocus(const Focused& moved) {
    moved_ = moved;
}

/**
 **/
boost::shared_ptr<Component> Engine::focused() const {
    return focused_.lock();
}

namespace {

/**
 * Collect what can be focused, in the order the draw walk reaches it.
 *
 * A flow box holds its children in the order it places them and a z index inside one
 * changes nothing, which is the rule Arranger::walk follows and the reason this cannot
 * simply sort everything by depth.
 **/
void focusable(const boost::shared_ptr<Component>& component,
    std::vector<boost::shared_ptr<Component>>* found) {
    if (!component || !component->visible() || !component->enabled()) {
        return;  // a hidden or disabled subtree is skipped whole, not just its root
    }
    if (component->focusable()) {
        found->push_back(component);
    }
    const std::vector<boost::shared_ptr<Component>>& children = component->children();
    if (dynamic_cast<const component::Box*>(component.get()) != nullptr ||
        inDrawOrder(children)) {
        for (const boost::shared_ptr<Component>& child : children) {
            focusable(child, found);
        }
        return;
    }
    for (const boost::shared_ptr<Component>& child : ordered(children)) {
        focusable(child, found);
    }
}

};  // namespace

/**
 **/
std::vector<boost::shared_ptr<Component>> Engine::tabOrder() const {
    std::vector<boost::shared_ptr<Component>> order;
    for (const boost::shared_ptr<Container>& holder : containers_) {
        if (!holder || !holder->visible()) {
            continue;
        }
        for (const boost::shared_ptr<Component>& component : holder->ordered()) {
            focusable(component, &order);
        }
    }
    return order;
}

/**
 **/
bool Engine::focusFirst() {
    const std::vector<boost::shared_ptr<Component>> order = tabOrder();
    if (order.empty()) {
        return false;
    }
    focus(order.front());
    return true;
}

/**
 **/
bool Engine::focusNext(bool forward) {
    const boost::shared_ptr<Component> was = focused_.lock();
    if (!was) {
        return false;
    }

    const std::vector<boost::shared_ptr<Component>> order = tabOrder();
    if (order.empty()) {
        return false;
    }
    const auto here = std::find(order.begin(), order.end(), was);
    if (here == order.end()) {
        // what held the focus is no longer reachable - hidden, disabled or taken out of the
        // tree since it took it - so there is no place in the order to move on from, and the
        // walk starts again rather than leaving the focus somewhere tab cannot get it back
        focus(forward ? order.front() : order.back());
        return true;
    }
    if (order.size() < 2) {
        return false;
    }
    const std::size_t at = static_cast<std::size_t>(here - order.begin());
    const std::size_t next = forward ? (at + 1) % order.size()
        : (at + order.size() - 1) % order.size();
    focus(order[next]);
    return true;
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
