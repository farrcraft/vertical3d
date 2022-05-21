/**
 * Pong! - (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_PONG_CONTROLLER
#define INCLUDED_PONG_CONTROLLER

#include <vertical3d/hookah/Window.h>
#include <luxa/ComponentManager.h>
#include <luxa/menu/Menu.h>
#include <vertical3d/input/KeyboardDevice.h>
#include <vertical3d/input/MouseDevice.h>
#include <vertical3d/gui/InputEventAdapter.h>

class PongScene;
class PongRenderer;

/**
 * The core game controller class
 */
class PongController
{
	public:
		PongController(const std::string & path);

		bool run();

		bool exec(const v3D::CommandInfo & command, const std::string & param);
		bool execUI(const v3D::CommandInfo & command, const std::string & param);

	protected:
		void setMenuItemDefaults(const boost::shared_ptr<Luxa::Menu> & menu);

	private:
		boost::shared_ptr<PongScene> scene_;
		boost::shared_ptr<PongRenderer> renderer_;
		boost::shared_ptr<Luxa::ComponentManager> vgui_;
		boost::shared_ptr<Hookah::Window> window_;
		boost::shared_ptr<v3D::KeyboardDevice> keyboard_;
		boost::shared_ptr<v3D::MouseDevice> mouse_;
		boost::shared_ptr<v3D::CommandDirectory> directory_;
		boost::shared_ptr<v3D::InputEventAdapter> listenerAdapter_;
};

#endif // INCLUDED_PONG_CONTROLLER
