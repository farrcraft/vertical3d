/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#ifndef INCLUDED_LUXA_FONTSTYLEPROPERTY
#define INCLUDED_LUXA_FONTSTYLEPROPERTY

#include "StyleProperty.h"

#include <string>

namespace Luxa
{

	/**
	 * A style property that defines a font.
	 */
	class FontStyleProperty : public StyleProperty
	{
		public:
			FontStyleProperty(const std::string & name, const std::string & src);
			~FontStyleProperty();

			bool italics() const;
			bool bold() const;
			std::string face() const;
			unsigned int size() const;
			std::string source() const;

		private:
			std::string source_;
			bool italics_;
			bool bold_;
			std::string face_;
			unsigned int size_;
	};

}; // end namespace Luxa


#endif // INCLUDED_LUXA_FONTSTYLEPROPERTY
