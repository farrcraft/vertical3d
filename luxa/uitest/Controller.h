#ifndef INCLUDED_UITEST_CONTROLLER
#define INCLUDED_UITEST_CONTROLLER

#include "../luxa/ComponentManager.h"

#include <vertical3d/command/CommandDirectory.h>
#include <vertical3d/hookah/Window.h>
#include <vertical3d/input/KeyboardDevice.h>
#include <vertical3d/input/MouseDevice.h>
#include <vertical3d/gui/InputEventAdapter.h>
#include <vertical3d/font/FontCache.h>

class Controller
{
	public:
		Controller();
		~Controller();

		bool run();

	private:
		boost::shared_ptr<Luxa::ComponentManager> cm_;
		boost::shared_ptr<Hookah::Window> window_;
		boost::shared_ptr<v3D::KeyboardDevice> keyboard_;
		boost::shared_ptr<v3D::MouseDevice> mouse_;
		boost::shared_ptr<v3D::InputEventAdapter> listenerAdapter_;
		boost::shared_ptr<v3D::CommandDirectory> directory_;
		boost::shared_ptr<v3D::FontCache> fontCache_;
};


#endif // INCLUDED_UITEST_CONTROLLER
