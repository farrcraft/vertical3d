/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_LUXA_OVERLAY
#define INCLUDED_LUXA_OVERLAY

#include <vertical3d/3dtypes/Color3.h>

namespace Luxa
{

	/**
	 * The Overlay is used to specify the background attributes of the UI.
	 * Since the UI can be drawn on top of other content we don't necessarily want to do the
	 * normal clearing of the background color when drawing. The overlay is used to tell the
	 * renderer how to handle the background. It can be transparent (do nothing), or a normal 
	 * background color.
	 */
	class Overlay
	{
		public:
			/**
			 * Overlay Mode
			 */ 
			enum Mode
			{
				MODE_TRANSPARENT,
				MODE_COLOR
			};

			/**
			 * Default Constructor
			 */
			Overlay();
			/**
			 * Construct an overlay with the given mode
			 * @param m the mode to use
			 */
			Overlay(Mode m);
			/**
			 * Construct a color overlay with the given color
			 * @param c the color of the overlay
			 */
			Overlay(const v3D::Color3 & c);
			/**
			 * Set the overlay's color
			 * @param c the new color
			 */
			void color(const v3D::Color3 & c);
			/**
			 * Get the overlay's color
			 * @return the current overlay color
			 */
			v3D::Color3 color() const;
			/**
			 * Get the overlay mode
			 * @return the current overlay mode
			 */
			Mode mode() const;

		private:
			Mode mode_;
			v3D::Color3 color_;
	};

}; // end namespace Luxa

#endif // INCLUDED_LUXA_OVERLAY
