/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <map>
#include <set>
#include <string>
#include <vector>

namespace v3d::moya::ri::internal {

/**
 * The objects a world makes, and the names they hold. A name a kind already holds gets a number
 * after it. A new world removes every object the world before it made.
 **/
class Objects final {
 public:
    /** @param scene the scene the objects are made in, which outlives this **/
    explicit Objects(scene::Scene * scene);

    /** Make an object in the world, which clear() removes. **/
    scene::Handle make(const std::string & kind, const std::string & name);

    /** A name no object of the kind holds: the one given, or it with a number after it. **/
    std::string unique(const std::string & kind, const std::string & base);

    /**
     * A name for a new object of a kind: the one given, or the request's and a count when the
     * one given is empty. Either is made unique().
     **/
    std::string name(const std::string & kind, const std::string & request, const std::string & given);

    /** Remove every object made, and forget the names and counts. **/
    void clear();

 private:
    scene::Scene * scene_;
    std::vector<scene::Handle> made_;
    std::map<std::string, unsigned int> counts_;
    std::map<std::string, std::set<std::string>> taken_;
};

};  // namespace v3d::moya::ri::internal
