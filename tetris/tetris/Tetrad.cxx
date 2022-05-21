#include "Tetrad.h"

#include <iostream>
#include <cassert>
#include <algorithm>
#include <cstring>

Tetrad::Tetrad(const ShapeInfo & shape) : orientation_(0), shape_(shape), initialized_(true)
{
}

Tetrad::Tetrad() : orientation_(0), initialized_(false)
{
}

Tetrad & Tetrad::operator=(const Tetrad & t)
{
	orientation_ = t.orientation_;
	initialized_ = t.initialized_;
	shape_ = t.shape_;
	return *this;
}

bool Tetrad::initialized() const
{
	return initialized_;
}

void Tetrad::move(int dx, int dy)
{
	if (dx < 0)
	{
		assert(position_.first + offset(OFFSET_X) > 0);
	}
	if (dy < 0)
	{
		assert(position_.second + offset(OFFSET_Y) > 0);
	}

	position_.first += dx;
	position_.second += dy;
}

void Tetrad::rotate(RotationDirection dir)
{
	unsigned short rotated[4][4];

	for (unsigned int i = 0; i < 4; i++)
	{
		for (unsigned int j = 0; j < 4; j++)
		{
			int new_x, new_y;
			if (dir == CLOCKWISE) // clockwise
			{
				new_x = i + (3 - (i+j));
				new_y = i;
				rotated[new_x][new_y] = shape_.layout_[i][j];
			}
			else // ccw
			{
				rotated[i][j] = shape_.layout_[i][j];
			}

		}
	}

	// copy rotation before shift first so we get correct offsets for the shift later
	//std::copy(rotated, rotated + 16, shape_.layout_);
	memcpy(shape_.layout_, rotated, sizeof(rotated));

	// FIXME!!!
	unsigned short shifted[4][4];
	unsigned int off_x = offset(Tetrad::OFFSET_Y);
	unsigned int off_y = offset(Tetrad::OFFSET_X);
	unsigned shift_x, shift_y;

	for (unsigned int i = 0; i < 4; i++)
	{
		for (unsigned int j = 0; j < 4; j++)
		{
			shift_x = i + off_x;
			shift_y = j + off_y;
			if (shift_x < 4 && shift_y < 4)
				shifted[i][j] = rotated[shift_x][shift_y];
			else
				shifted[i][j] = 0;
		}
	}

	//std::copy(shifted, shifted + 16, shape_.layout_);
	memcpy(shape_.layout_, shifted, sizeof(shifted));
}

void Tetrad::position(PositionType p)
{
	position_ = p;
}

Tetrad::PositionType Tetrad::position() const
{
	return position_;
}

unsigned int Tetrad::orientation() const
{
	return orientation_;
}

void Tetrad::orientation(unsigned int o)
{
	orientation_ = o;
}

const Tetrad::ShapeInfo & Tetrad::shape() const
{
	return shape_;
}

unsigned int Tetrad::offset(OffsetAxis dir) const
{
	unsigned int x = 3, y = 3;

	for (unsigned int i = 0; i < 4; i++)
	{
		for (unsigned int j = 0; j < 4; j++)
		{
			if (shape_.layout_[i][j] == 1)
			{
				if (x > j)
					x = j;
				if (y > i)
					y = i;
			}
		}
	}
	if (dir == OFFSET_X)
		return x;
	return y;
}

unsigned int Tetrad::width() const
{
	unsigned int min = 0, max = 0;
	bool found_edge;

	for (unsigned int i = 0; i < 4; i++)
	{
		found_edge = false;
		for (unsigned int j = 0; j < 4; j++)
		{
			if (shape_.layout_[i][j] == 1) // flip i & j here for height instead
			{
				if (!found_edge)
				{
					found_edge = true;
					if (min > j)
						min = j;
				}
				if (max < j)
					max = j;
			}
		}
	}
	return (max - min + 1);
}

unsigned int Tetrad::height() const
{
	unsigned int min = 0, max = 0;
	bool found_edge;

	for (unsigned int i = 0; i < 4; i++)
	{
		found_edge = false;
		for (unsigned int j = 0; j < 4; j++)
		{
			if (shape_.layout_[j][i] == 1) // flip i & j here for width instead
			{
				if (!found_edge)
				{
					found_edge = true;
					if (min > j)
						min = j;
				}
				if (max < j)
					max = j;
			}
		}
	}
	return (max - min + 1);
}
