/**
 * Pong! - (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_PONG_BALL
#define INCLUDED_PONG_BALL

#include <glm/glm.hpp>

class Ball
{
	public:
		Ball();

		void direction(const glm::vec2 & dir);
		glm::vec2 direction() const;

		glm::vec2 position() const;
		void position(const glm::vec2 & pos);

		void move();

		float size() const;
		void size(float s);

	private:
		glm::vec2 position_;
		glm::vec2 direction_;
		float size_;
};

#endif // INCLUDED_PONG_BALL

