/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#ifndef INCLUDED_V3D_FONT2D
#define INCLUDED_V3D_FONT2D

#include "../image/Texture.h"

#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>
#include <map>

namespace v3D
{

	/**
	 * 2D GL Font
	 */
	class Font2D
	{
		public:
			Font2D();
			Font2D(const std::string & face, unsigned int size);
			~Font2D();

			/**
			 * Enumeration of font style attributes
			 */
			typedef enum
			{
				STYLE_BOLD,		/**< Bold font style **/
				STYLE_ITALICS,	/**< Italics font style **/
				STYLE_UNDERLINE,/**< Underline font style **/
				STYLE_STRIKEOUT	/**< Strikeout font style **/
			} FontStyle;

			/**
			 * Representation of a single character glyph
			 */
			struct Glyph
			{
				
				float x1_;
				float y1_;
				float x2_;
				unsigned int advance_;
			};

			/**
			 * Set the font style attributes
			 * @param attribs the style attributes to set
			 */
			void style(unsigned int attribs);
			/**
			 * Get the style attributes
			 * @return the current style attributes
			 */
			unsigned int style(void) const;
			/**
			 * Get the font type size
			 * @return the type size
			 */
			int size(void) const;
			/**
			 * Set the font type size
			 * The font must be built after setting the size before the change will be visible.
			 * @param s the new font size
			 */
			void size(int s);
			/**
			 * Get the typeface of the font
			 * @return the name of the typeface
			 */
			std::string typeface(void) const;
			/**
			 * Set the font typeface
			 * The font must be built after setting the face name before the change will be visible.
			 * @param face the typeface
			 */
			void typeface(const std::string & face);
			/**
			 * Generate the GL objects necessary for the font with the current face and size.
			 * @return whether the font was build successfully
			 */
			bool build(void);
			/**
			 * Get the width of the string
			 * @param str the string to get the width of
			 * @return the width
			 */
			unsigned int width(const std::string & str) const;
			/**
			 * Get the line height of the font
			 * @return the height
			 */
			unsigned int height(void) const;
			/**
			 * Get the width of a single character
			 * @param charcode the character to get the width of
			 * @return the width
			 */
			unsigned int width(unsigned char charcode) const;
			/**
			 * Access the underlying font character map texture
			 * @return a pointer to the texture
			 */
			boost::shared_ptr<Texture> texture(void) const;
			/**
			 * Get the texture height
			 * @return the texture height
			 */
			float textureHeight(void) const;
			/**
			 * Get a single character glyph
			 * @param charcode the character to lookup
			 * @return a pointer to a single character glyph
			 */
			const Glyph * glyph(unsigned char charcode) const;

		private:
			std::map<unsigned char, Glyph> 	glyphs_;
			boost::shared_ptr<Texture> 		texture_;

			float 			tex_line_height_; 	// _line
			unsigned int 	line_height_; 		// _height

			std::string 	typeface_;
			int 			size_;
			unsigned int 	style_;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_FONT2D
