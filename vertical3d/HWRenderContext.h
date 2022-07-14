/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../v3dlibs/core/SceneVisitor.h"
#include "../v3dlibs/core/Scene.h"
#include "../v3dlibs/type/Camera.h"
#include "../v3dlibs/brep/BRep.h"

namespace v3d {

    /**
     * This is a hardware (OpenGL) renderer responsible for rendering the contents of a scene database
     * It is currently implemented as a single scene visitor. However, it might eventually become more of a
     * manager for multiple specialized scene visitors instead of an actual visitor itself.
     */
    class HWRenderContext : public v3d::core::SceneVisitor {
     public:
            typedef enum PolyRenderMode {
                POLY_RENDER_POINTS,
                POLY_RENDER_LINES,
                POLY_RENDER_FILL
            } PolyRenderMode;

            typedef enum PolyShadeMode {
                POLY_SHADE_FLAT,
                POLY_SHADE_SMOOTH
            } PolyShadeMode;

            typedef enum PolyCullMode {
                POLY_CULL_NONE,
                POLY_CULL_FRONT,
                POLY_CULL_BACK,
                POLY_CULL_BOTH
            } PolyCullMode;

            /**
             * SceneVisitor override
             * @param mesh the mesh to visit
             */
            void visit(const boost::shared_ptr<v3d::brep::BRep> & mesh);

            /**
             * Prepare to render with the given camera.
             * @param camera the scene camera 
             */
            void prepare(const boost::shared_ptr<v3d::type::Camera> & camera);

            /**
             *	render an entire scene
             * @param scene the scene to render
             */
            void render(const boost::shared_ptr<v3d::core::Scene> & scene);

            /**
             *	render a single mesh
             * @param mesh the mesh to render
             */
            void render(const boost::shared_ptr<v3d::brep::BRep> & mesh);

            /**
             * Resize the context.
             * @param width the new context width
             * @param height the new context height
             */
            void resize(int width, int height);

     private:
            PolyRenderMode frontRenderMode_;
            PolyRenderMode backRenderMode_;
            PolyShadeMode shadeMode_;
            PolyCullMode cullMode_;
    };

};  // namespace v3d
