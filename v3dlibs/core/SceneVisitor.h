/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "../brep/BRep.h"

#include <boost/shared_ptr.hpp>

namespace v3d::core {
    /**
     * The interface for a scene visitor.
     * Visitors may be interested in certain kinds of nodes within a scene database.
     */
    class SceneVisitor {
     public:
            /*
            a generic version may specify a bunch of node types (or the types might be specified somewhere else) with pow2
            values that can be combined into a single flag value:

            typedef enum NodeType { MESH_SHAPE_NODE = 1, LIGHT_NODE = 2, CAMERA_NODE = 4 } NodeType;

            with a constructor that specifies what kind of nodes this visitor wants to visit:
            SceneVisitor(unsigned int nodeFlags);

            and a generic visit method:
            virtual void visit(const boost::shared_ptr<ComponentNode> & node) = 0;

            specific visitors can be derived, implementing whatever functionality they require.
            */
            virtual ~SceneVisitor() { }

            virtual void visit(const boost::shared_ptr<v3d::brep::BRep> & mesh) = 0;
    };

};  // namespace v3d::core
