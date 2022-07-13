#ifndef INCLUDED_XML_CDSECT
#define INCLUDED_XML_CDSECT

#include <iostream>
#include <fstream>

#include "Node.h"

#include "../DOM/CDATASection.h"

namespace XML
{

	class CDSect : public Node, DOM::CDATASection
	{
		public:
			CDSect();

			void setData(const std::wstring & str);

			bool sync(std::wofstream & xml_file);
	};

}; // end namespace XML

#endif // INCLUDED_XML_CDSECT
