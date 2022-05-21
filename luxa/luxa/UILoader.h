/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_LUXA_UILOADER
#define INCLUDED_LUXA_UILOADER

#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>

namespace Luxa
{
	// forward declarations
	class ComponentManager;
	class Component;
	class Menu;
	class MenuItem;
	class Theme;
	class Style;

	/**
	 * Load the ui from a property tree description.
	 */
	class UILoader
	{
		public:
			/**
			 * Load the ui from a property tree
			 * @param tree the property tree to load from
			 * @param cm a pointer to the component manager to be used
			 * @return whether the configuration was successfully loaded or not
			 */
			bool load(const boost::property_tree::ptree & tree, ComponentManager * cm);

		protected:
			bool loadTheme(const boost::property_tree::ptree & theme_node, ComponentManager * cm);
			bool loadStyle(const boost::property_tree::ptree & style_node, boost::shared_ptr<Theme> the_theme);
			bool loadStyleProperty(const boost::property_tree::ptree & property_node, boost::shared_ptr<Style> the_style, const std::string & property_type);

			// load all of the textures specified within image style properties
			bool loadTextures(ComponentManager * cm);

			bool loadMenu(const boost::property_tree::ptree & menu_node, ComponentManager * cm);
			void loadMenuItem(ComponentManager * cm, const boost::property_tree::ptree & tree_item, boost::shared_ptr<Menu> parent);
			bool loadLabel(const boost::property_tree::ptree & label_node, ComponentManager * cm);
			bool loadButton(const boost::property_tree::ptree & button_node, ComponentManager * cm);
			bool loadFont(const boost::property_tree::ptree & font_node, ComponentManager * cm);
			bool loadIcon(const boost::property_tree::ptree & icon_node, ComponentManager * cm);
			void loadDefaultComponentAttributes(const boost::property_tree::ptree & node, boost::shared_ptr<Component> component);
	};

}; // end namespace Luxa

#endif // INCLUDED_LUXA_UILOADER
