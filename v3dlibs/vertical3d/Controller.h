#ifndef INCLUDED_V3DAPP_CONTROLLER
#define INCLUDED_V3DAPP_CONTROLLER

#include "ViewPort.h"
#include "../core/Scene.h"
#include "../hookah/Window.h"
#include "../gui/InputEventAdapter.h"
#include "../command/CommandDirectory.h"
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
		v3D::CommandDirectory directory_;
		boost::shared_ptr<v3D::InputEventAdapter> listenerAdapter_;

		boost::shared_ptr<v3D::Scene> scene_;
		boost::shared_ptr<v3D::ViewPort> view_;

		std::map<std::string, boost::shared_ptr<v3D::Tool> > tools_;
		boost::shared_ptr<v3D::Tool> activeTool_;

		Luxa::ComponentManager cm_;
};


#endif // INCLUDED_V3DAPP_CONTROLLER
