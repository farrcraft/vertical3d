#ifndef INCLUDED_V3D_HWRENDERCONTEXT
#define INCLUDED_V3D_HWRENDERCONTEXT

#include <vertical3d/core/SceneVisitor.h>
#include <vertical3d/3dtypes/Camera.h>

namespace v3D
{
	class BRep;
	class Scene;

	/**
	 * This is a hardware (OpenGL) renderer responsible for rendering the contents of a scene database
	 * It is currently implemented as a single scene visitor. However, it might eventually become more of a
	 * manager for multiple specialized scene visitors instead of an actual visitor itself.
	 */
	class HWRenderContext : public SceneVisitor
	{
		public:

			typedef enum PolyRenderMode
			{
				POLY_RENDER_POINTS,
				POLY_RENDER_LINES,
				POLY_RENDER_FILL
			} PolyRenderMode;

			typedef enum PolyShadeMode
			{
				POLY_SHADE_FLAT,
				POLY_SHADE_SMOOTH
			} PolyShadeMode;

			typedef enum PolyCullMode
			{
				POLY_CULL_NONE,
				POLY_CULL_FRONT,
				POLY_CULL_BACK,
				POLY_CULL_BOTH
			} PolyCullMode;

			/**
			 * SceneVisitor override
			 * @param mesh the mesh to visit
			 */
			void visit(const boost::shared_ptr<BRep> & mesh);

			/**
			 * Prepare to render with the given camera.
			 * @param camera the scene camera 
			 */
			void prepare(const boost::shared_ptr<Camera> & camera);

			/**
			 *	render an entire scene
			 * @param scene the scene to render
			 */
			void render(const boost::shared_ptr<Scene> & scene);

			/**
			 *	render a single mesh
			 * @param mesh the mesh to render
			 */
			void render(const boost::shared_ptr<BRep> & mesh);

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

}; // end namespace v3D

#endif // INCLUDED_V3D_HWRENDERCONTEXT
