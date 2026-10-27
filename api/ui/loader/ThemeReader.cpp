/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ThemeReader.h"

#include <api/asset/Json.h>
#include <api/log/Logger.h>
#include <api/ui/style/Button.h>
#include <api/ui/style/Property.h>
#include <api/ui/style/Style.h>
#include <api/ui/style/Theme.h>
#include <api/ui/style/property/Color.h>
#include <api/ui/style/property/Image.h>
#include <api/ui/style/property/Number.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Members.h"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/value.hpp>
#include <boost/make_shared.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::loader {

namespace {

/**
 * A colour's value: four numbers, RGBA.
 * @return the colour, or nothing when the value is missing or is not four numbers
 **/
std::optional<glm::vec4> colour(const boost::json::object& property) {
    const boost::json::array* values = v3d::asset::readArray(property, "value");
    if (values == nullptr || values->size() != 4) {
        return std::nullopt;
    }
    glm::vec4 result(0.0F);
    for (std::size_t index = 0; index < values->size(); index++) {
        const boost::json::value& value = (*values)[index];
        if (!value.is_number()) {
            return std::nullopt;
        }
        result[static_cast<glm::vec4::length_type>(index)] = static_cast<float>(value.to_number<double>());
    }
    return result;
}

};  // namespace

ThemeReader::ThemeReader(const boost::shared_ptr<v3d::log::Logger>& logger) : logger_(logger) {
}

bool ThemeReader::read(const boost::json::object& doc) {
    const boost::json::array* themes = requiredArray(*logger_, doc, "themes", "A ui document");
    if (themes == nullptr) {
        return false;
    }
    for (const boost::json::value& item : *themes) {
        const boost::json::object* theme = requiredObject(*logger_, item, "The themes");
        if (theme == nullptr || !readTheme(*theme)) {
            return false;
        }
    }

    if (const std::optional<std::string> named = optionalString(*logger_, doc, "theme")) {
        active_ = *named;
        // Interface makes the named theme active. It is checked here, so that a document naming
        // a theme it does not carry is refused, not half loaded
        const bool known = std::any_of(themes_.begin(), themes_.end(),
            [this](const boost::shared_ptr<style::Theme>& theme) { return theme->name() == active_; });
        if (!known) {
            logger_->get()->error("Unknown active theme [{}] in config", active_);
            return false;
        }
    }
    return true;
}

std::vector<boost::shared_ptr<style::Theme>>& ThemeReader::themes() noexcept {
    return themes_;
}

const std::string& ThemeReader::active() const noexcept {
    return active_;
}

bool ThemeReader::readTheme(const boost::json::object& entry) {
    const std::optional<std::string> named = requiredString(*logger_, entry, "name", "A theme");
    if (!named) {
        return false;
    }
    const boost::shared_ptr<style::Theme> theme = boost::make_shared<style::Theme>(*named);

    // a theme with no styles in it is legal and draws in the defaults
    if (const boost::json::array* styles = optionalArray(*logger_, entry, "styles")) {
        for (const boost::json::value& item : *styles) {
            const boost::json::object* style = requiredObject(*logger_, item, "A theme's styles");
            if (style == nullptr || !readStyle(*style, theme)) {
                return false;
            }
        }
    }

    themes_.push_back(theme);
    return true;
}

bool ThemeReader::readStyle(const boost::json::object& entry, const boost::shared_ptr<style::Theme>& theme) {
    const std::optional<std::string> className = requiredString(*logger_, entry, "class", "A style");
    const std::optional<std::string> styleName = requiredString(*logger_, entry, "name", "A style");
    if (!className || !styleName) {
        return false;
    }

    boost::shared_ptr<style::Style> target;
    if (*className == "button") {
        // a button is drawn differently in each of its states, so its styles are told apart
        // by the state as well as by the name. "disabled" and "inactive" both name the look
        // of a component that is not enabled
        const style::Button::State state = optionalChoice(*logger_, entry, "state", {
            { "normal", style::Button::State::Normal },
            { "hover", style::Button::State::Hover },
            { "press", style::Button::State::Press },
            { "disabled", style::Button::State::Disabled },
            { "inactive", style::Button::State::Disabled } }, style::Button::State::Normal);
        target = boost::make_shared<style::Button>(*styleName, state);
    } else {
        target = boost::make_shared<style::Style>(style::Selector{.name = *styleName, .className = *className});
    }

    if (!readProperties(entry, target)) {
        return false;
    }
    theme->addStyle(target);
    return true;
}

bool ThemeReader::readProperties(const boost::json::object& entry, const boost::shared_ptr<style::Style>& target) {
    // each array holds one property class, so the array a property is written in, not a
    // field on the property, decides what it is read as
    static const char* const sections[] = { "colors", "numbers", "images" };

    for (const char* const section : sections) {
        const boost::json::array* properties = optionalArray(*logger_, entry, section);
        if (properties == nullptr) {
            continue;
        }
        for (const boost::json::value& item : *properties) {
            const boost::json::object* property = requiredObject(*logger_, item, "A style's properties");
            if (property == nullptr) {
                return false;
            }
            const std::optional<std::string> name = requiredString(*logger_, *property, "name", "A style property");
            if (!name) {
                return false;
            }
            std::string propertyClass;
            const boost::shared_ptr<style::Property> loaded = readProperty(section, *property, *name, &propertyClass);
            if (!loaded) {
                return false;
            }
            target->addProperty(loaded, propertyClass);
        }
    }
    return true;
}

boost::shared_ptr<style::Property> ThemeReader::readProperty(const std::string& section,
    const boost::json::object& property, const std::string& name, std::string* propertyClass) {
    if (section == "colors") {
        const std::optional<glm::vec4> value = colour(property);
        if (!value) {
            logger_->get()->error("The style colour [{}] needs a value of four numbers", name);
            return nullptr;
        }
        *propertyClass = "color";
        return boost::make_shared<style::property::Color>(name, *value);
    }
    if (section == "numbers") {
        const std::optional<double> value = v3d::asset::readNumber(property, "value");
        if (!value) {
            logger_->get()->error("The style number [{}] has no value", name);
            return nullptr;
        }
        *propertyClass = "number";
        return boost::make_shared<style::property::Number>(name, static_cast<float>(*value));
    }
    const std::optional<std::string> source = requiredString(*logger_, property, "source", "A style image");
    if (!source) {
        return nullptr;
    }
    *propertyClass = "image";
    return boost::make_shared<style::property::Image>(style::property::Image::Named{.name = name, .source = *source});
}

};  // namespace v3d::ui::loader
