/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <functional>
#include <span>

#include <boost/json/object.hpp>

namespace v3d::asset {

/**
 * One migration step: it takes a document of one version to the next, and returns whether it
 * succeeded. It reads and writes only the document it is given, because that is a copy
 * readForward() discards if a later step fails.
 **/
typedef std::function<bool(boost::json::object&)> Migration;

/**
 * What reading a document forward found. The caller decides what to do about each.
 **/
enum class Reading {
    Current,    // already at this build's version, and untouched
    Migrated,   // migrated forward to this build's version
    Newer,      // a later build wrote it, so it is untouched and must not be overwritten
    Refused     // no version, one that is not a whole number above zero, a missing or failed step
};

/**
 * Walk a document forward to this build's version, one step at a time.
 *
 * The document's version is the whole number at its root under "version". chain[0] takes
 * version 1 to 2, so a build at version n has n - 1 steps. Migration runs on a copy, stamps
 * the version after every step, and assigns the copy back only when every step succeeded, so
 * a refused document is exactly what was handed in.
 *
 * @param document the document to read, which is replaced only on Reading::Migrated
 * @param current the version this build writes, from 1
 * @param chain the steps, oldest first
 * @return what was found
 **/
Reading readForward(boost::json::object* document, int current, std::span<const Migration> chain);

};  // namespace v3d::asset
