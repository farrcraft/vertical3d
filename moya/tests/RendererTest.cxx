#include <boost/test/unit_test.hpp>

#include "../libmoya/Renderer.h"

BOOST_AUTO_TEST_CASE( renderer_test )
{
    v3D::Moya::Renderer renderer;
    // create a new render context
    renderer.createRenderContext("default context");
    // get a reference to the context
    v3D::Moya::RenderContext & rc = renderer.activeRenderContext();
    // set the image resolution on the context
    rc.imageResolution(1280, 960, 1.0f);
    // get another reference to the context from the renderer
    v3D::Moya::RenderContext rc2 = renderer.activeRenderContext();
    // test that the resolution we set is the same on the new reference
    unsigned int width;
    width = rc2.imageWidth();
    BOOST_CHECK_EQUAL(width, 1280);

    // destroy the context
    renderer.destroyActiveRenderContext();
    // get another context (creates a new context)
    rc = renderer.activeRenderContext();
    // make sure the resolution is the default
    width = rc.imageWidth();
    BOOST_CHECK_EQUAL(width, 320);
}