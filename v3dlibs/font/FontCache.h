#ifndef INCLUDED_V3D_FONTCACHE
#define INCLUDED_V3D_FONTCACHE

#include "Font2D.h"

#include <boost/shared_ptr.hpp>

#include <map>
#include <string>

namespace v3D
{
	/**
	 * A container of 2D fonts.
	 */
	class FontCache
	{
		public:
			FontCache();
			~FontCache();

			/**
			 * Load a font
			 * @param name a name used for referencing the font
			 * @param face the typeface of the font
			 * @param size the point size of the font
			 */
			bool load(const std::string & name, const std::string & face, unsigned int size);
			/**
			 * Get the named font.
			 * @param name the name of the font to get
			 * @return a pointer to the font or NULL if the font hasn't been loaded
			 */
			boost::shared_ptr<Font2D> get(const std::string & name);

		private:
			std::map<std::string, boost::shared_ptr<Font2D> > fonts_;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_FONTCACHE
