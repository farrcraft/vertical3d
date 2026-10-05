/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/log/Logger.h>

#include <boost/shared_ptr.hpp>

namespace v3d::asset::media {

/**
 * Register the loaders for pictures and models with a manager, and the extensions each is
 * found by.
 *
 * engine::Engine does this for the manager it builds. A manager built anywhere else loads
 * none of these until something calls it.
 **/
void registerLoaders(Manager& manager, const boost::shared_ptr<v3d::log::Logger>& logger);

};  // namespace v3d::asset::media
