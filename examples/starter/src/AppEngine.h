/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/engine/Engine.h>
#include <api/render/realtime/Canvas.h>
#include <api/render/realtime/Engine3D.h>

#include <string>

#include <boost/shared_ptr.hpp>

/**
 * An application is a subclass of the game engine. The engine calls start() once before
 * the loop, tick() and render() every frame, and release() once after it.
 **/
class AppEngine final : public v3d::engine::Engine {
 public:
    explicit AppEngine(const std::string& path);

    bool tick(unsigned int delta) override;
    bool render() override;

 protected:
    v3d::engine::Features features() const override;
    bool start() override;
    bool release() override;

 private:
    boost::shared_ptr<v3d::render::realtime::Engine3D> renderer_;
    v3d::render::realtime::Canvas canvas_;
};
