/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/type/camera/Profile.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/Curve.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Visitor.h>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "RIBExportVisitor.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::render {

/**
 * Keeps moya's retained scene in step with the editor's. It makes the objects the RIB export
 * makes through moya's RIB client, so the two describe alike. The options, the camera and the
 * output come first. Each mesh is a prototype and an instance, each run of a curve's curves is
 * a prototype and an instance, and each light is one of UsdLux's. Every surface takes one
 * material of RI's `matte`, and a scene with no light gets the camera's distant light.
 *
 * **An update changes only what moved.** Each object is kept by its kind and name, which come
 * from the item, and a parameter is set only when its value differs from the one set before.
 * Moving one item sets its instance's transformation alone. An object no item makes any more is
 * removed. An update that changes anything commits once.
 **/
class SceneTranslator final {
 public:
    /**
     * @param scene made with moya::scene::kinds(), which outlives the translator and is filled by
     *        nothing else
     * @param guides whether the options ask for the beauty denoised, so that the film holds the
     *        albedo and the normal that guide the denoiser. The RIB export asks for neither.
     **/
    explicit SceneTranslator(v3d::moya::scene::Scene* scene, bool guides = false);

    /**
     * Bring the scene in step with an evaluation of the editor's scene, seen from a camera, as the
     * output's settings render it. An item renders do not show is left out.
     *
     * @return whether anything changed, and so whether a batch was committed
     **/
    bool update(const scene::Scene& scene, const scene::Evaluation& evaluation, const v3d::type::camera::Profile& from,
        const RIBExportVisitor::Output& output);

 private:
    /**
     * Walks the editor's scene, making each item's objects. Lights are made as they are met, and
     * the camera's light at the end when there were none.
     **/
    class Walk final : public scene::Visitor {
     public:
        explicit Walk(SceneTranslator* translator);

        unsigned int lights() const;

        void visit(const boost::shared_ptr<scene::item::Mesh>& item, const scene::Placed& placed) override;
        void visit(const boost::shared_ptr<scene::item::Camera>& camera, const scene::Placed& placed, const scene::item::Camera::Lens& lens) override;
        void visit(const boost::shared_ptr<scene::item::Group>& group, const scene::Placed& placed) override;
        void visit(const boost::shared_ptr<scene::item::Curve>& curve, const scene::Placed& placed) override;
        void visit(const boost::shared_ptr<scene::item::Light>& light, const scene::Placed& placed, const scene::item::Light::Emission& emission) override;
        void leave(const boost::shared_ptr<scene::item::Item>& item) override;

     private:
        /** An instance of a prototype at an item's placement, with the one material. **/
        void place(const v3d::moya::scene::Handle& prototype, const std::string& name, const scene::Placed& placed);

        SceneTranslator* translator_;
        unsigned int lights_ = 0;

        /** The curve basis each block the walk is inside holds, outermost first. **/
        std::vector<std::string> bases_ { "bezier" };
    };

    /** The object of a kind and name, made the first time an update asks for it. **/
    v3d::moya::scene::Handle object(const std::string& kind, const std::string& name);

    /** Set a parameter when its value differs from the one set before. **/
    void put(const v3d::moya::scene::Handle& object, const std::string& parameter, const v3d::moya::scene::Value& value);

    /**
     * Set a parameter an update set before back to its kind's fallback, for a value an item no
     * longer gives. Nothing is set when the parameter was never set or already holds it.
     **/
    void reset(const v3d::moya::scene::Handle& object, const std::string& parameter);

    /** The one material every surface takes, made when the first surface needs it. **/
    v3d::moya::scene::Handle material();

    v3d::moya::scene::Scene* scene_;
    bool guides_;
    std::map<std::pair<std::string, std::string>, v3d::moya::scene::Handle> objects_;
    /** The values set on each object, by its handle's index and generation. **/
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::map<std::string, v3d::moya::scene::Value>> values_;
    std::set<std::pair<std::string, std::string>> seen_;
    bool changed_ = false;
};

};  // namespace v3d::editor::render
