/**
 * Pong! - (c) Joshua Farr <j.wgasa@gmail.com>
 */
#ifndef INCLUDED_PONG_PADDLE
#define INCLUDED_PONG_PADDLE

#include <glm/glm.hpp>

class Paddle
{
    public:
        Paddle();

		void color(const glm::vec3 & color1, const glm::vec3 & color2);
        void move(float delta);
		void position(const float pos);
		void offset(const float off);
		float offset() const;
		float position() const;

		glm::vec3 color1() const;
		glm::vec3 color2() const;

		void reset();

		bool up();
		bool down();
		int score();
		float length();
		float size();

		void up(bool k);
		void down(bool k);
		void score(int s);

    private:
        float position_;
		float offset_;
        glm::vec3 color1_;
        glm::vec3 color2_;

		float length_;
		float size_;

		bool up_;
		bool down_;
		int score_;
};

#endif // INCLUDED_PONG_PADDLE
