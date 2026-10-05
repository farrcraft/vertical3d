/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/log/Logger.h>

#include <boost/shared_ptr.hpp>

namespace v3d::audio {

/**
 * Register the loader for a sound with a manager, and the extensions it is found by.
 *
 * An app that plays sound calls this on its manager; one that does not never links the
 * mixer for it - ADR-0079.
 **/
void registerLoaders(v3d::asset::Manager& manager, const boost::shared_ptr<v3d::log::Logger>& logger);

};  // namespace v3d::audio
