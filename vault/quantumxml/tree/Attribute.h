#ifndef INCLUDED_XML_ATTRIBUTE
#define INCLUDED_XML_ATTRIBUTE

#include <fstream>
#include <vector>

#include "Reference.h"

#include "../DOM/Attr.h"

namespace XML
{

	class Attribute : public DOM::Attr
	{
		public:
			Attribute();
			Attribute(const std::wstring & name);
			~Attribute();

			void name(const std::wstring & str);
			void value(const std::wstring & str);
			void reference(const Reference & ref);

			bool sync(std::wofstream & file);

		private:
			std::vector<Reference> _valrefs;
	};

}; // end namespace XML

#endif // INCLUDED_XML_ATTRIBUTE
