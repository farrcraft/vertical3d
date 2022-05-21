/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_LUXA_LABEL
#define INCLUDED_LUXA_LABEL

#include "Component.h"

#include <string>

namespace Luxa
{

	/**
	 * A vGUI Label
	 **/
	class Label : public Component
	{
		public:
			Label(ComponentManager *cm);
			~Label();

			/**
			 * Set the label text
			 * @param txt the new text value
			 */
			void text(const std::string & txt);
			/**
			 * Get the current label text
			 * @return the current text value
			 */
			std::string text(void) const;
			/**
			 * Draw the label
			 * @param renderer the renderer to use for drawing
			 * @param theme the active theme
			 */
			void draw(ComponentRenderer * renderer, const boost::shared_ptr<Theme> & theme) const;

		private:
			std::string text_;
//			std::string _font;
	};

}; // end namespace Luxa

#endif // INCLUDED_LUXA_LABEL
