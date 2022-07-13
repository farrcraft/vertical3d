#ifndef INCLUDED_XML_ATTLIST
#define INCLUDED_XML_ATTLIST

#include "AttDef.h"

namespace XML
{

	class AttList
	{
		public:
			AttList(const std::wstring & elem);

			std::wstring name(void) const;
			void attdef(const AttDef & def);

		private:
			std::wstring _name;
			std::vector<AttDef>	_attdefs;
	};

}; // end namespace XML

#endif // INCLUDED_XML_ATTLIST
