#ifndef INCLUDED_XML_NOTATION
#define INCLUDED_XML_NOTATION

#include "ExternalID.h"
#include "../DOM/Notation.h"

namespace XML
{

	/**
		The notation name is stored in the DOM::Node name property.

	**/
	class Notation : public DOM::Notation
	{
		public:
			Notation(const std::wstring & name);
			virtual ~Notation();

			void externalID(const ExternalID & extid);

		//private:
		//	ExternalID	_extId;
		//	std::wstring _notationId;
	};

}; // end namespace XML

#endif // INCLUDED_XML_NOTATION
