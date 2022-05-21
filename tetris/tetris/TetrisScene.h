#ifndef INCLUDED_TETRIS_SCENE
#define INCLUDED_TETRIS_SCENE

#include "GameBoard.h"

class TetrisScene
{
	public:
		TetrisScene();

		void tick(unsigned int delta);
		void resize(int width, int height);

		GameBoard * board();

		bool debug() const;
		void debug(bool dbg);

	private:
		GameBoard board_;
		int score_;
		bool debug_;
};

#endif // INCLUDED_TETRIS_SCENE
