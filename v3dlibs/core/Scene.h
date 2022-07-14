/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <vector>

#include "SceneVisitor.h"

#include "../type/CameraProfile.h"
#include "../type/Camera.h"
#include "../brep/BRep.h"

#include <boost/shared_ptr.hpp>

namespace v3d::core {
    /**
     *	The Scene class.
     *	The scene traditionally represents the root of a scene graph type structure. In a component based design
     *  it might make sense that the scene represents more of a database type structure instead. The interface for
     *  the scene might then be methods for performing queries on the database - e.g. adding new component nodes,
     *  selecting existing component nodes, deleting component nodes, updating component nodes (Updating might not 
     *  be a required action since nodes can be updated directly after they are selected). It might also be possible
     *  to replace the selection concept with the visitor pattern instead - the action visits the object instead.
     *  
     *	A scene stores a collection of nodes
     *		geometry (meshes, curves ...) or shape nodes
     *		cameras
     *		lights
     *		groups
     */
    class Scene {
     public:
            /*
               allow storing raw meshes temporarily. this might eventually use something 
               like a ShapeNode that can be inserted with a generic addNode method.
            */
            void addBRep(const boost::shared_ptr<v3d::brep::BRep> & brep);
            /*
                similar to the addBRep method but for camera profiles. except instead of a shape node
                this might be for something called e.g. a TemplateNode. 
            */
            void addCameraProfile(const v3d::type::CameraProfile & profile);

            /**
             * Accept a scene visitor to iterate over the scene
             * @param visitor a pointer to the scene visitor
             */
            void accept(SceneVisitor * visitor);

     private:
            std::vector< boost::shared_ptr<v3d::brep::BRep> > meshes_;
            std::vector< boost::shared_ptr<v3d::type::Camera> > cameras_;
            std::vector<v3d::type::CameraProfile> cameraProfiles_;
    };

};  // namespace v3d::core
