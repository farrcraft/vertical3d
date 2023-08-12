/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Theme.h"

namespace v3d::ui::style {

Theme::Theme(const std::string& str) : name_(str) {
}

Theme::~Theme() {
}

std::string_view Theme::name() const {
	return name_;
}

void Theme::addStyle(boost::shared_ptr<Style> style) {
	styles_.push_back(style);
}


std::vector< boost::shared_ptr<Style> > Theme::getStyleSet(const std::string& name, const std::string& class_name) const {
	std::vector< boost::shared_ptr<Style> > matching_styles;
	std::vector< boost::shared_ptr<Style> >::const_iterator iter = styles_.begin();
	for (; iter != styles_.end(); iter++)
	{
		if ((name == "" || (*iter)->name() == name) && (class_name == "" || (*iter)->className() == class_name))
			matching_styles.push_back((*iter));
	}
	return matching_styles;
}

};  // namespace v3d::ui::style