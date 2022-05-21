#ifndef INCLUDED_V3DAPP_CONTROLLER
#define INCLUDED_V3DAPP_CONTROLLER

#include "ViewPort.h"
#include <vertical3d/core/Scene.h>
#include <vertical3d/hookah/Window.h>
#include <vertical3d/gui/InputEventAdapter.h>
#include <vertical3d/command/CommandDirectory.h>
#include "Tool.h"

#include <luxa/ComponentManager.h>

#include <boost/property_tree/ptree.hpp>

class Controller
{
	public:
		Controller();
		~Controller();

		bool run();
		bool exec(const v3D::CommandInfo & command, const std::string & param);

	protected:
		void activate_tool(const std::string & name);
		void load_camera_profiles(const boost::property_tree::ptree & tree);

	private:
		boost::shared_ptr<Hookah::Window> window_;
		boost::shared_ptr<v3D::KeyboardDevice> keyboard_;
		boost::shared_ptr<v3D::MouseDevice> mouse_;
		boost::shared_ptr<v3D::CommandDirectory> directory_;
		boost::shared_ptr<v3D::InputEventAdapter> listenerAdapter_;

		boost::shared_ptr<v3D::Scene> scene_;
		boost::shared_ptr<v3D::ViewPort> view_;

		std::map<std::string, boost::shared_ptr<v3D::Tool> > tools_;
		boost::shared_ptr<v3D::Tool> activeTool_;

		boost::shared_ptr<Luxa::ComponentManager> cm_;
};


#endif // INCLUDED_V3DAPP_CONTROLLER
