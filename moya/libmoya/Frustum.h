/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Plane.h"

#include "../../api/type/AABBox.h"

#include <string>
#include <map>

namespace v3d::moya {
    class Frustum {
        public:
            Frustum();
            Frustum(const glm::mat4x4 & projection);
            ~Frustum();
            
            enum HitClassification
            {
                OUTSIDE		= -1,
                CROSSING	=  0,
                INSIDE		=  1
            };
    
            // extract the 6 standard clipping planes from a projection matrix
            void extract(const glm::mat4x4 & projection);
    
            // normalize all planes in frustum
            void normalize(void);
    
            int intersect(const v3d::type::AABBox & aabb);
//				int intersect(const Sphere & sphere);
            /*
                clip polygon against frustum - modifies passed polygon structure
                this does a sutherland-hodgeman clip against each plane
                reyes doesn't perform any clipping operations
            */
            void clip(boost::shared_ptr<Polygon> poly);
    
        private:
            std::map<std::string, Plane> 	_clippingPlanes;
    };
};