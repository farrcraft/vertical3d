/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <boost/shared_ptr.hpp>

#include <string>
#include <map>
#include <vector>

namespace Luxa
{
	class StyleProperty;

	/**
	 * A vGUI Style.
	 * An example of a style (in the XML Configuration Language):
	 *	\<style class="button" name="default" state="normal">
	 *		\<image name="top-left" source="top-left.png" />
	 *		\<image name="top-right" source="top-right.png" />
	 *		\<image name="bottom-left" source="bottom-left.png" />
	 *		\<image name="bottom-right" source="bottom-right.png" />
	 *		\<image name="left" source="left.png" />
	 *		\<image name="right" source="right.png" />
	 *		\<image name="top" source="top.png" />
	 *		\<image name="bottom" source="bottom.png" />
	 *		\<image name="center" source="bg.png" />
	 *		\<font face="Courier" size="18" bold="true" italics="true" align="center" />
	 *	\</style>
	 */
	class Style
	{
		public:
			Style(const std::string & str, const std::string & class_name);
			virtual ~Style();

			/**
			 * Get the name of the style.
			 * @return the style name
			 */
			std::string name() const;
			/**
			 * Get the class name of the style.
			 * @return the style's class name
			 */
			std::string className() const;
			/**
			 * Get a style property.
			 * @param name the property name
			 * @param class_name the property class
			 * @return a pointer to the named style property
			 */
			boost::shared_ptr<StyleProperty> property(const std::string & name, const std::string & class_name) const;
			/**
			 * Add a style property to the style
			 * @param prop the new property to be added
			 * @param class_name the class of the new property
			 */
			void addProperty(boost::shared_ptr<StyleProperty> prop, const std::string & class_name);
			/**
			 * Get a collection of style properties
			 * If no name or class is specified then no filtering of that parameter will be done. This allows getting the
			 * subsets that consist of all styles, all of the same name, and all of the same class.
			 * @param name the name of the properties to get
			 * @param class_name the class of the properties to get
			 * @return a collection of matching properties
			 */
			std::vector< boost::shared_ptr<StyleProperty>  > getPropertySet(const std::string & name, const std::string & class_name) const;

		protected:
			void className(const std::string & str);

		private:
			std::string name_;
			std::string className_;
			std::map <std::pair<std::string, std::string>, boost::shared_ptr<StyleProperty> > properties_; // key is pair<name, class>

	};

}; // end namespace Luxa

