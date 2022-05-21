#ifndef INCLUDED_TETRIS_RENDERER
#define INCLUDED_TETRIS_RENDERER

#include "Piece.h"
#include "Tetrad.h"

#include <vertical3d/hookah/Window.h>
#include <vertical3d/gl/GLTexture.h>
#include <vertical3d/font/FontCache.h>
#include <boost/shared_ptr.hpp>

class TetrisScene;

class TetrisRenderer
{
	public:
		TetrisRenderer(boost::shared_ptr<TetrisScene> scene);

		void draw(Hookah::Window * window);
		void resize(int width, int height);

		bool drawBoard();
		void drawPiece(const Piece & piece);
		void drawTetrad(const Tetrad & tetrad, bool dbg);

	private:
		boost::shared_ptr<TetrisScene> scene_;
		std::map<std::string, boost::shared_ptr<v3D::GLTexture> >	textures_;
		boost::shared_ptr<v3D::FontCache> fonts_;
};

#endif // INCLUDED_TETRIS_RENDERER
