#include "Controller.h"

#include "../luxa/Icon.h"
#include "../luxa/Button.h"
#include "../luxa/Label.h"
#include "../luxa/UILoader.h"

#include <vertical3d/hookah/Hookah.h>

//#include "../hookah/BindLoader.h"

#include <boost/bind.hpp>
#include <boost/property_tree/xml_parser.hpp>

Controller::Controller()
{
	// create new app window and set caption
	window_ = Hookah::Create3DWindow(800, 600);

	// create input devices
	keyboard_ = boost::dynamic_pointer_cast<v3D::KeyboardDevice, v3D::InputDevice>(Hookah::CreateInputDevice("keyboard"));
	mouse_ = boost::dynamic_pointer_cast<v3D::MouseDevice, v3D::InputDevice>(Hookah::CreateInputDevice("mouse"));

	directory_ = boost::shared_ptr<v3D::CommandDirectory>(new v3D::CommandDirectory());
	fontCache_ = boost::shared_ptr<v3D::FontCache>(new v3D::FontCache());

	// register app's command directory as an observer of input device events
	listenerAdapter_.reset(new v3D::InputEventAdapter(keyboard_, mouse_));
	listenerAdapter_->connect(boost::dynamic_pointer_cast<v3D::EventListener>(directory_).get());

	// [TODO] - need a font cache here still
	cm_ = boost::shared_ptr<Luxa::ComponentManager> (new Luxa::ComponentManager(fontCache_, directory_));

	// register component manager as an observer of input device events
	keyboard_->addEventListener(boost::dynamic_pointer_cast<v3D::KeyboardEventListener, Luxa::ComponentManager>(cm_).get(), "luxa::cm");
	mouse_->addEventListener(cm_.get(), "luxa::cm");

	// add devices to window
	window_->addInputDevice("keyboard", keyboard_);
	window_->addInputDevice("mouse", mouse_);

	// set window caption
	window_->caption("uitest");

	// load config file into a property tree
	boost::property_tree::ptree ptree;
	boost::property_tree::read_xml("vgui.xml", ptree);

	/*
	// load key binds from the property tree
	BindLoader loader;
	loader.load(ptree, directory());
	*/

	/*
	// create the game menus
	boost::shared_ptr<Luxa::MenuStack> menu(new Luxa::MenuStack());
	// load menus from config ptree
	menu->load(ptree);
	*/

/*
the overlay manager will need to hook into input events and route them to the various components as needed.

there are several scenarios:
- rendering the primary scene directly to the canvas with components rendered on top
- rendering the primary scene through luxa components (e.g. viewports)
*/
	// register event listeners
	window_->addPostDrawListener(boost::bind(&Luxa::ComponentManager::draw, boost::ref(cm_), _1));
	window_->addResizeListener(boost::bind(&Luxa::ComponentManager::resize, boost::ref(cm_), _1, _2));
	window_->addTickListener(boost::bind(&Luxa::ComponentManager::tick, boost::ref(cm_), _1));

	// set default managed area to match canvas size
	cm_->resize(window_->width(), window_->height());

	// set a black colored overlay
	v3D::Color3 overlay_color(0.0f, 0.0f, 0.0f);
	Luxa::Overlay overlay(overlay_color);
	cm_->renderer()->overlay(overlay);

	// load UI from the root config property tree
	Luxa::UILoader ui_loader;
	ui_loader.load(ptree, cm_.get());

	// create free ui objects directly on canvas procedurally
	boost::shared_ptr<Luxa::Button> button(new Luxa::Button(cm_.get()));
	button->label("Test Button");
	button->size(glm::vec2(100., 50.));
	button->position(glm::vec2(100., 100.));
	cm_->addOverlayComponent(button);

	// create an icon that uses the default ui font's texture as its source image
	boost::shared_ptr<v3D::Font2D> font = cm_->fonts()->get("ui-text-font");
	boost::shared_ptr<v3D::GLTexture> font_texture(new v3D::GLTexture(*font->texture()));
	boost::shared_ptr<Luxa::Icon> icon(new Luxa::Icon(font_texture, cm_.get()));
	icon->position(glm::vec2(300., 20.));
	cm_->addOverlayComponent(icon);

	// create a text label
	boost::shared_ptr<Luxa::Label> label(new Luxa::Label(cm_.get()));
	label->text("Test Label");
	label->position(glm::vec2(100., 300.));
	cm_->addOverlayComponent(label);
}

Controller::~Controller()
{
}

bool Controller::run()
{
	return window_->run(Hookah::Window::EVENT_HANDLING_BLOCKING, Hookah::Window::DRAW_DIRTY);
}

