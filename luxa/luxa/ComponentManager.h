/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_LUXA_COMPONENTMANAGER
#define INCLUDED_LUXA_COMPONENTMANAGER

#include "ComponentRenderer.h"

#include <vertical3d/image/Texture.h>
#include <vertical3d/hookah/Window.h>
#include <vertical3d/input/MouseEventListener.h>
#include <vertical3d/input/KeyboardEventListener.h>
#include <vertical3d/command/CommandDirectory.h>

#include <vector>
#include <map>
#include <string>

namespace Luxa
{

	class Component;
	class Theme;

	/**
	 * A manager for vGUI components.
	 */
	class ComponentManager : 
		public v3D::MouseEventListener,
		public v3D::KeyboardEventListener
	{
		public:
			ComponentManager(boost::shared_ptr<v3D::FontCache> fc, boost::shared_ptr<v3D::CommandDirectory> directory);

			/**
			 * Drawing callback renders all of the managed components.
			 * @param window the rendering surface being drawn to
			 */
			void draw(Hookah::Window * window);
			/**
			 * Frame update callback.
			 * @param delta time since the last tick event
			 */
			void tick(unsigned int delta);
			/**
			 * Resize event callback.
			 * @param width new canvas width
			 * @param height new canvas height
			 */
			void resize(int width, int height);
			/**
			 * Add a new component to the manager
			 * @param component the component to manage
			 */
			void addOverlayComponent(boost::shared_ptr<Component> component);
			/**
			 * Add a new theme to the manager
			 * @param theme the theme to add
			 */
			void addTheme(boost::shared_ptr<Theme> theme);
			/**
			 * Get the width of the managed component area.
			 * This is usually the same as the window canvas width
			 * @return the current width
			 */
			unsigned int width() const;
			/**
			 * Get the height of the managed component area.
			 * This is usually the same as the window canvas height
			 * @return the current height
			 */
			unsigned int height() const;
			/**
			 * Get the font cache used by the vgui system.
			 * @return a pointer to the font cache
			 */
			boost::shared_ptr<v3D::FontCache> fonts() const;
			void addTexture(const std::string & name, const boost::shared_ptr<v3D::Texture> & texture);
			boost::shared_ptr<v3D::Image> loadImage(const std::string & filename);
			/**
			 * Get the active theme.
			 * @return a pointer to the active theme
			 */
			boost::shared_ptr<Theme> activeTheme() const;
			/**
			 * Set the active theme
			 * @param name the name of the new active theme
			 */
			void activeTheme(const std::string & name);
			/**
			 * Get the gui renderer
			 * @return a pointer to the renderer
			 */
			ComponentRenderer * renderer();
			/**
			 * Get a component by its name.
			 * @param name the name of the component to get
			 * @return a pointer to the named component
			 */
			boost::shared_ptr<Component> getComponent(const std::string & name) const;
			/**
			 * Get a component at the given intersection point
			 *
			 * @param point the hit point to test against
			 * @return the component at the specified point
			 */
			boost::shared_ptr<Component> intersect(glm::vec2 point) const;

			bool toggleComponentVisibility(const std::string & name);

			/**
			 * Call a command
			 * 
			 * @param cmd the name of the command
			 * @param scope the scope of the command
			 * @param param a parameter for the command
			 * @return whether the command was successfully called
			 */
			bool execCommand(const std::string & cmd, const std::string & scope, const std::string param);

		protected:
			// overrides from MouseEventListener
			void motion(unsigned int x, unsigned int y);
			void buttonPressed(unsigned int button);
			void buttonReleased(unsigned int button);
			// overrides from KeyboardEventListener base
			void keyPressed(const std::string & key);
			void keyReleased(const std::string & key);
			void notify(const v3D::EventInfo & e);

		private:
			std::vector< boost::shared_ptr<Component> > components_;
			std::map < std::string, boost::shared_ptr<Theme> > themes_;
			std::string activeTheme_;
			std::map<std::string, boost::shared_ptr<v3D::Texture> >	textures_;
			boost::shared_ptr<v3D::CommandDirectory> directory_;
			ComponentRenderer renderer_;
			glm::vec2 mouse_;
			boost::shared_ptr<Component> focused_;
	};


}; // end namespace Luxa

#endif // INCLUDED_LUXA_COMPONENTMANAGER
