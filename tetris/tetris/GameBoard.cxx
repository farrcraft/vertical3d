#include "GameBoard.h"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>

#include <log4cxx/logger.h>

GameBoard::GameBoard() : rows_(20), cols_(10), fallRate_(800), 
						fastFallMultiplier_(8), fastFall_(false), debug_(false)
{
	// initialize the entire board with blank pieces
	for (unsigned int i = 0; i < rows_; i++)
	{
		std::vector<Piece> row;
		for (unsigned int j = 0; j < cols_; j++)
		{
			Piece p;
			row.push_back(p);
		}
		pieces_.push_back(row);
	}

	// load tetrad shape bitmaps / colors
	loadShapeInfo();

	// random seed
	srand((unsigned)time(0));
}

bool GameBoard::dropTetrad()
{
	fastFall_ = !fastFall_;
	return fastFall_;
}

bool GameBoard::debug() const
{
	return debug_;
}

void GameBoard::debug(bool dbg)
{
	debug_ = dbg;
}

unsigned int GameBoard::columns() const
{
	return cols_;
}

unsigned int GameBoard::rows() const
{
	return rows_;
}

Piece GameBoard::piece(unsigned int col, unsigned int row) const
{
	if (row >= pieces_.size() || col >= pieces_[row].size())
	{
		return Piece();
	}
	return pieces_[row][col];
}

Tetrad GameBoard::currentTetrad() const
{
	return currentTetrad_;
}

Tetrad GameBoard::nextTetrad() const
{
	return nextTetrad_;
}

Tetrad & GameBoard::currentTetrad()
{
	return currentTetrad_;
}

Tetrad GameBoard::randomTetrad()
{
	// pick a shape
	size_t shapeCount = shapes_.size();
	size_t num = rand() % shapeCount;
	log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("tetris"));
	LOG4CXX_DEBUG(logger, "GameBoard::randomTetrad - selecting random tretrad # [" << num << "] from [" << shapeCount << "]");
	
	Tetrad piece(shapes_[num]);
	// and an initial rotation
	unsigned int rot = rand() % 3;
	for (unsigned int i = 0; i < rot; i++)
	{
		piece.rotate(Tetrad::CLOCKWISE);
	}

	return piece;
}

void GameBoard::loadShapeInfo()
{
	log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("tetris"));
	std::string filename = "pieces/shapes.txt";
	std::ifstream shapes_file(filename.c_str(), std::ifstream::in);

	while (shapes_file.good())
	{
		if (shapes_file.peek() == '\n')
		{
			shapes_file.get();
		}
		Tetrad::ShapeInfo info;
		for (unsigned int i = 0; i < 4; i++)
		{
			for (unsigned int j = 0; j < 4; j++)
			{
				if (!shapes_file.good())
				{
					break;
				}
				char val = shapes_file.get();
				info.layout_[i][j] = atoi(&val);
				if (shapes_file.peek() == '\n')
				{
					shapes_file.get();
				}
			}
		}
		if (!shapes_file.good())
		{
			break;
		}
		shapes_file >> info.color_;
		LOG4CXX_DEBUG(logger, "GameBoard::loadShapeInfo - got shape info color [" << info.color_ << "]");
		shapes_.push_back(info);
		if (shapes_file.peek() == '\n')
		{
			shapes_file.get();
		}
	}
}

void GameBoard::spawnTetrad()
{
	Tetrad::PositionType pos;

	if (!nextTetrad_.initialized())
	{
		nextTetrad_ = randomTetrad();
	}

	// reset default fall speed flag
	fastFall_ = false;

	currentTetrad_ = nextTetrad_;
	nextTetrad_ = randomTetrad();
	// place it centered at the top of the board
	pos = Tetrad::PositionType(cols_ / 2, 0);
	currentTetrad_.position(pos);
}

void GameBoard::update(unsigned int delta)
{
	// ms remaining until current tetrad falls again
	static int nextMove = fallRate_;

	Tetrad::PositionType pos;

	// no current tetrad so spawn a new one
	if (!currentTetrad_.initialized())
	{
		spawnTetrad();
	}
	else
	{
		pos = currentTetrad_.position();

		nextMove -= delta;
		// drop the current piece down one spot
		if (nextMove <= 0)
		{
			// figure out the offset
			unsigned int h = currentTetrad_.height();
			unsigned int top_offset = currentTetrad_.offset(Tetrad::OFFSET_Y);
			unsigned int bottom_offset = 3 - h - top_offset;

			if (pos.second <= (rows_ - bottom_offset)) 
			{
				currentTetrad_.move(0, 1);
				if (!fastFall_)
				{
					nextMove = fallRate_;
				}
				else
				{
					nextMove = fallRate_ / fastFallMultiplier_;
				}
			}
		}

		// collision detection
		Tetrad::ShapeInfo shape = currentTetrad_.shape();
		unsigned int i = 0, j = 0;

		// current tetrad cannot fall any further? (blocked by existing pieces)
		bool blocked = false;
		for (i = 0; i < 4; i++)
		{
			for (j = 0; j < 4; j++)
			{
				if ((pos.second + i + 1) < rows_ &&
					shape.layout_[i][j] == 1 && 
					pieces_[pos.second + i + 1][pos.first + j].color() != Piece::COLOR_EMPTY)
				{
					blocked = true;
				}
			}
		}
		if (blocked)
		{
			// break tetrad into individual pieces on the board
			for (i = 0; i < 4; i++)
			{
				for (j = 0; j < 4; j++)
				{
					if (shape.layout_[i][j] == 1)
					{
						Piece p(shape.color_);
						pieces_[pos.second + i][pos.first + j] = p;
					}
				}
			}
			// respawn
			spawnTetrad();
		}

		// current tetrad reached bottom of board?
		unsigned int bottom = pos.second + currentTetrad_.height() + currentTetrad_.offset(Tetrad::OFFSET_Y);
		if (bottom >= rows_)
		{
			// break tetrad into individual pieces on the board
			for (i = 0; i < 4; i++)
			{
				for (j = 0; j < 4; j++)
				{
					if (shape.layout_[i][j] == 1)
					{
						Piece p(shape.color_);
						pieces_[pos.second + i][pos.first + j] = p;
					}
				}
			}
			// respawn
			spawnTetrad();
		}
	}
	// check for completed rows
	checkCompletedRows();
}

void GameBoard::checkCompletedRows(void)
{
	for (unsigned int i = 0; i < rows_; i++)
	{
		bool completed = false;
		unsigned int filled = 0;
		for (unsigned int j = 0; j < cols_; j++)
		{
			if (pieces_[i][j].color() != Piece::COLOR_EMPTY)
			{
				filled++;
			}
		}
		// clear out this row
		if (filled == cols_)
		{
			for (unsigned int j = 0; j < cols_; j++)
			{
				Piece p; // empty piece
				pieces_[i][j] = p;
			}
			// move contents of all above rows down one row
			for (unsigned m = i; m > 0; m--)
			{
				for (unsigned int n = 0; n < cols_; n++)
				{
					pieces_[m][n] = pieces_[m - 1][n];
				}
			}
		}
	}
}
