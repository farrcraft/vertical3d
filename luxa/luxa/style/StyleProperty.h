/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace Luxa
{
	/**
	 * A style property.
	 * This is a base property class that specific types can derive from.
	 */
	class StyleProperty
	{
		public:
			StyleProperty(const std::string & str);
			virtual ~StyleProperty();

			typedef enum
			{
				TOP,
				BOTTOM,
				LEFT,
				RIGHT,
				TOP_LEFT,
				BOTTOM_LEFT,
				TOP_RIGHT,
				BOTTOM_RIGHT,
				CENTER,
				NULL_ALIGNMENT
			} Alignment;

			/**
			 * Get alignment property value.
			 * @return the alignment setting
			 */
			Alignment align() const;
			/**
			 * Set the property alignment.
			 * @param a the new alignment
			 */
			void align(Alignment a);
			/**
			 * Get the property name.
			 * @return the property name
			 */
			std::string name() const;

		private:
			Alignment align_;
			std::string name_;
	};

}; // end namespace Luxa
