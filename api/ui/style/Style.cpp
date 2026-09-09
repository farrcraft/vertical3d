/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Style.h"

#include <api/ui/style/property/Color.h>
#include <api/ui/style/property/Number.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Property.h"

namespace v3d::ui::style {

Style::Style(const std::string& str, const std::string& class_name) : name_(str), className_(class_name) {
}

Style::~Style() {
}

std::string_view Style::name() const {
    return name_;
}

std::string_view Style::className() const {
    return className_;
}

void Style::className(const std::string& str) {
    className_ = str;
}

boost::shared_ptr<Property> Style::property(const std::string& name, const std::string& class_name) const {
    boost::shared_ptr<Property> prop;
    std::pair<std::string, std::string> key(name, class_name);
    std::map<std::pair<std::string, std::string>, boost::shared_ptr<Property> >::const_iterator iter = properties_.find(key);
    if (iter != properties_.end())
        prop = iter->second;
    return prop;
}

void Style::addProperty(boost::shared_ptr<Property> prop, const std::string& class_name) {
    std::pair<std::string, std::string> key(prop->name(), class_name);
    properties_[key] = prop;
}

std::vector< boost::shared_ptr<Property>  > Style::getPropertySet(const std::string& name, const std::string& class_name) const {
    std::map<std::pair<std::string, std::string>, boost::shared_ptr<Property> >::const_iterator iter = properties_.begin();
    std::vector< boost::shared_ptr<Property> > props;
    for (; iter != properties_.end(); iter++) {
        if ((name.empty() || (*iter).first.first == name) && (class_name.empty() || (*iter).first.second == class_name))
            props.push_back((*iter).second);
    }
    return props;
}

void readColour(const boost::shared_ptr<Style>& target, const std::string& name, glm::vec4* into) {
    if (!target) {
        return;
    }
    const boost::shared_ptr<property::Color> property =
        boost::dynamic_pointer_cast<property::Color>(target->property(name, "color"));
    if (property) {
        *into = property->value();
    }
}

void readMetric(const boost::shared_ptr<Style>& target, const std::string& name, float* into) {
    if (!target) {
        return;
    }
    const boost::shared_ptr<property::Number> property =
        boost::dynamic_pointer_cast<property::Number>(target->property(name, "number"));
    if (property) {
        *into = property->value();
    }
}

};  // namespace v3d::ui::style
