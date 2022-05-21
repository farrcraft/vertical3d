#ifndef INCLUDED_TETRIS_TETRAD
#define INCLUDED_TETRIS_TETRAD

#include <string>
#include <utility> // for std::pair

/**
 * A group of individual blocks forming a shape. We call the current 
 * (or any future) falling piece a tetrad.
 */
class Tetrad
{
	public:

		class ShapeInfo
		{
			public:
				unsigned short layout_[4][4]; // internal bitmap
				std::string color_; // texture name
		};

		typedef enum
		{
			OFFSET_X = 0,
			OFFSET_Y = 1,
			OFFSET_COL = 1,
			OFFSET_ROW = 0
		} OffsetAxis;

		typedef enum
		{
			CLOCKWISE,
			COUNTERCLOCKWISE
		} RotationDirection;

		typedef std::pair<unsigned int, unsigned int> PositionType;

		Tetrad();
		Tetrad(const ShapeInfo & shape);

		/**
		 * Set the position of the tetrad.
		 * @param p the new position
		 */
		void position(PositionType p);
		/**
		 * Move the tetrad by the specified amount.
		 * @param dx amount to move the tetrad in the x axis (number of columns)
		 * @param dy amount to move the tetrad in the y axis (number of rows)
		 */
		void move(int dx, int dy);
		/**
		 * Get the current tetrad position
		 * @return the current position
		 */
		PositionType position() const;
		unsigned int orientation() const;
		void orientation(unsigned int o);
		void rotate(RotationDirection dir);
		unsigned int width() const;
		unsigned int height() const;
		bool initialized() const;
		const ShapeInfo & shape() const;

		/**
		 * Get the offset of the shape from the top left of the 4x4 grid
		 * @param dir the offset axis to be returned
		 * @return the offset in the given axis
		 */
		unsigned int offset(OffsetAxis dir) const;

		Tetrad & operator=(const Tetrad & t);

	private:
		unsigned int orientation_; // 0 = 0, 1 = 90, 2 = 180, 3 = 270 degree rotation
		PositionType position_; // where on the board
//		unsigned int _width;
//		unsigned int _height;
		ShapeInfo shape_;
		bool initialized_;
};

#endif // INCLUDED_TETRIS_TETRAD

