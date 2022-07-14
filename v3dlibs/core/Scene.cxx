/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Scene.h"
#include "../brep/BRep.h"
#include "SceneVisitor.h"

namespace v3d::core {

    void Scene::addBRep(const boost::shared_ptr<v3d::brep::BRep>& brep) {
        meshes_.push_back(brep);
    }

    void Scene::accept(SceneVisitor* visitor) {
        std::vector< boost::shared_ptr<v3d::brep::BRep> >::iterator iter = meshes_.begin();
        for (; iter != meshes_.end(); iter++) {
            visitor->visit(*iter);
        }
    }

    void Scene::addCameraProfile(const v3d::type::CameraProfile& profile) {
        cameraProfiles_.push_back(profile);
    }

};  // namespace v3d::core
