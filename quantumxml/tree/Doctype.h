#ifndef INCLUDED_XML_DOCTYPE
#define INCLUDED_XML_DOCTYPE

#include <map>
#include <vector>

#include "ExternalID.h"
#include "ElementDecl.h"
#include "Misc.h"

#include "../DOM/DocumentType.h"

namespace XML
{

	/**
	* the internal representation of a document's DTD
	*/
	class Doctype : public DOM::DocumentType
	{
		public:
			Doctype(const std::wstring & name);

			ExternalID & externalID(void);
			void externalID(const ExternalID & extid);
			std::map<std::wstring, ElemDecl> & elementDecl(void);
			boost::shared_ptr<Misc> misc(void);
			void addMisc(boost::shared_ptr<Misc> m);
			bool sync(std::wofstream & xml_file);

		private:
			ExternalID	_externalID;
			// map the element's name to it's corresponding tree structure
			std::map<std::wstring, ElemDecl>	_elementDecl;
			//std::vector<Misc*> _misc;
			boost::shared_ptr<Misc> _misc;
	};

}; // end namespace XML

#endif // INCLUDED_XML_DOCTYPE