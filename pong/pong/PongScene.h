/**
 * Pong! - (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_PONG_SCENE
#define INCLUDED_PONG_SCENE

#include "Ball.h"
#include "Paddle.h"
#include "GameState.h"

#include <vertical3d/audio/SoundEngine.h>

#include <boost/property_tree/ptree.hpp>

class PongScene
{
	public:
		PongScene(const boost::property_tree::ptree & ptree, const std::string & assetPath);
		~PongScene();

		void tick(unsigned int delta);
		void resize(int width, int height);

		void reset();

		Ball & ball();
		Paddle & left();
		Paddle & right();
		GameState & state();

	private:
        Ball ball_;
        Paddle left_, right_;
		GameState gameState_;
		int width_, height_;
		v3D::SoundEngine soundEngine_;
};


#endif // INCLUDED_PONG_SCENE
