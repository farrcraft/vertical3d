/**
 * Pong! - (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_PONG_RENDERER
#define INCLUDED_PONG_RENDERER

#include "PongScene.h"

#include <vertical3d/hookah/Window.h>
#include <vertical3d/gl/Canvas.h>
#include <vertical3d/gl/TextureFontRenderer.h>
#include <vertical3d/font/TextureFontCache.h>
#include <vertical3d/font/FontCache.h>

#include <boost/shared_ptr.hpp>

class AssetLoader;

class PongRenderer
{
	public:
		PongRenderer(boost::shared_ptr<PongScene> scene, boost::shared_ptr<AssetLoader> & loader);

		void draw(Hookah::Window * window);
		void resize(int width, int height);

		void drawBall();
		void drawPaddle(const Paddle & paddle);

		boost::shared_ptr<v3D::FontCache> fonts() const;

	private:
		boost::shared_ptr<PongScene> scene_;
		boost::shared_ptr<v3D::FontCache> fonts_;
		boost::shared_ptr<v3D::Canvas> canvas_;
		v3D::TextureTextBuffer::Markup markup_;
		boost::shared_ptr<v3D::TextureFontRenderer> fontRenderer_;
		boost::shared_ptr<v3D::TextureFontCache> fontCache_;
};

#endif // INCLUDED_PONG_RENDERER
