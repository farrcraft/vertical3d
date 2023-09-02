/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Container.h"
#include "style/Theme.h"

#include "../asset/Json.h"
#include "../log/Logger.h"

#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::ui {

	class Engine {
	public:
		bool load(const boost::shared_ptr<v3d::log::Logger>& logger, boost::shared_ptr<v3d::asset::Json>& config);

	private:
		std::vector<boost::shared_ptr<Container>> containers_;
		std::vector<boost::shared_ptr<style::Theme>> themes_;
	};

};  // namespace v3d::ui
