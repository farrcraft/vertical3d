/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Interface.h"

#include <api/event/Router.h>
#include <api/log/Logger.h>
#include <api/ui/DrawOrder.h>
#include <api/ui/Image.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Icon.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/style/Style.h>
#include <api/ui/style/Theme.h>
#include <api/ui/style/property/Image.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Component.h"
#include "Container.h"
#include "Loader.h"

#include <boost/make_shared.hpp>

namespace v3d::ui {

Interface::Interface(const boost::shared_ptr<v3d::event::Router>& router, const boost::shared_ptr<entt::dispatcher>& dispatcher,
    const boost::shared_ptr<v3d::log::Logger>& logger) :
    router_(router), dispatcher_(dispatcher), logger_(logger) {
}

const boost::shared_ptr<entt::dispatcher>& Interface::dispatcher() const noexcept {
    return dispatcher_;
}

bool Interface::load(const boost::json::object& config) {
    Loader loader(router_, dispatcher_, logger_);
    if (!loader.load(config)) {
        return false;
    }
    containers_ = std::move(loader.containers());
    themes_ = std::move(loader.themes());

    // the first theme loaded is active unless the document names one, so a config with a
    // single theme needs no field for it
    activeTheme_ = themes_.empty() ? nullptr : themes_.front();
    if (!loader.active().empty()) {
        activeTheme(loader.active());
    }
    return true;
}

/**
 **/
std::size_t Interface::resolveThemeImages(const Resolve& resolve) {
    std::size_t resolved = 0;
    for (const boost::shared_ptr<style::Theme>& theme : themes_) {
        for (const boost::shared_ptr<style::Style>& target : theme->getStyleSet("", "")) {
            for (const boost::shared_ptr<style::Property>& property : target->getPropertySet("", "image")) {
                const boost::shared_ptr<style::property::Image> image =
                    boost::dynamic_pointer_cast<style::property::Image>(property);
                if (!image) {
                    continue;
                }
                const v3d::ui::Image resolvedImage = resolve(std::string(image->source()));
                if (!resolvedImage.valid()) {
                    logger_->get()->error("Unable to resolve the ui image [{}]", image->source());
                    continue;
                }
                image->image(resolvedImage);
                resolved++;
            }
        }
    }
    return resolved;
}

std::size_t Interface::resolveComponentImages(const Resolve& resolve, const boost::shared_ptr<Component>& component) {
    // a container accepts a null entry, and a null component has no image to resolve
    if (!component) {
        return 0;
    }
    std::size_t resolved = 0;

    const boost::shared_ptr<component::Icon> icon = boost::dynamic_pointer_cast<component::Icon>(component);
    const boost::shared_ptr<component::Button> button = boost::dynamic_pointer_cast<component::Button>(component);
    // a toolbar's buttons are not its children, so the loop over children below does not
    // reach them
    const boost::shared_ptr<component::Toolbar> bar = boost::dynamic_pointer_cast<component::Toolbar>(component);
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

    // Loader adds a nested component to its parent, not to the container, so an icon inside a
    // panel or a box is reached only through this loop
    for (const boost::shared_ptr<Component>& child : component->children()) {
        resolved += resolveComponentImages(resolve, child);
    }
    return resolved;
}

std::size_t Interface::resolveContainerImages(const Resolve& resolve) {
    std::size_t resolved = 0;
    for (const boost::shared_ptr<Container>& container : containers_) {
        for (const boost::shared_ptr<Component>& component : container->components()) {
            resolved += resolveComponentImages(resolve, component);
        }
    }
    return resolved;
}

std::size_t Interface::resolveImages(const Resolve& resolve) {
    if (!resolve) {
        return 0;
    }
    return resolveThemeImages(resolve) + resolveContainerImages(resolve);
}

/**
 **/
template <typename T>
bool Interface::resolveIcon(const Resolve& resolve, const std::string& source, const boost::shared_ptr<T>& target) {
    // a component naming no image is not a failure - most of them name none
    if (source.empty()) {
        return false;
    }
    const v3d::ui::Image image = resolve(source);
    if (!image.valid()) {
        logger_->get()->error("Unable to resolve the ui image [{}]", source);
        return false;
    }
    target->image(image);
    return true;
}

/**
 **/
boost::shared_ptr<style::Theme> Interface::theme(const std::string_view& name) const {
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
void Interface::focus(const boost::shared_ptr<Component>& component) {
    // a component that is not focusable, or not usable, is treated as null
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
    // called after both components' flags are set, so a listener that calls focused() sees
    // the finished move
    if (moved_) {
        moved_(wanted);
    }
}

/**
 **/
void Interface::onFocus(const Focused& moved) {
    moved_ = moved;
}

/**
 **/
boost::shared_ptr<Component> Interface::focused() const {
    return focused_.lock();
}

/**
 **/
bool Interface::reachable(const boost::shared_ptr<Component>& component) const {
    if (!component) {
        return false;
    }
    const std::vector<boost::shared_ptr<Component>> order = tabOrder();
    return std::find(order.begin(), order.end(), component) != order.end();
}

namespace {

/**
 * Collect what can be focused, in draw order. forEachDrawn skips a tab page that is not
 * chosen, so a control on one is left out.
 **/
void focusable(const boost::shared_ptr<Component>& component,
    std::vector<boost::shared_ptr<Component>>* found) {
    if (!component || !component->visible() || !component->enabled()) {
        return;  // a hidden or disabled subtree is skipped whole, not just its root
    }
    if (component->focusable()) {
        found->push_back(component);
    }
    forEachDrawn(*component, [found](const boost::shared_ptr<Component>& child) { focusable(child, found); });
}

};  // namespace

/**
 **/
std::vector<boost::shared_ptr<Component>> Interface::tabOrder() const {
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
bool Interface::focusFirst() {
    const std::vector<boost::shared_ptr<Component>> order = tabOrder();
    if (order.empty()) {
        return false;
    }
    focus(order.front());
    return true;
}

/**
 **/
bool Interface::focusNext(bool forward) {
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
        // the focused component is not reachable: it was hidden, disabled or taken out of the
        // tree. There is no place in the order to move on from, so the order starts again at
        // one end
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
boost::shared_ptr<style::Theme> Interface::activeTheme() const {
    return activeTheme_;
}

/**
 **/
bool Interface::activeTheme(const std::string_view& name) {
    const boost::shared_ptr<style::Theme> found = theme(name);
    if (!found) {
        logger_->get()->error("No such ui theme [{}]", name);
        return false;
    }
    activeTheme_ = found;
    return true;
}

boost::shared_ptr<Container> Interface::container(const std::string_view& name) {
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
const std::vector<boost::shared_ptr<Container>>& Interface::containers() const noexcept {
    return containers_;
}

};  // namespace v3d::ui
