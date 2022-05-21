#ifndef INCLUDED_XML_ENTITY
#define INCLUDED_XML_ENTITY

#include "ExternalID.h"
#include "Reference.h"

#include <vector>

namespace XML
{

	class Entity
	{
		public:
			enum EntityType 
			{ 
				IntGE, 
				ExtGEParsed, 
				ExtGEUnparsed, 
				IntPE, 
				ExtPE, 
				IntPredefined, 
				Unknown 
			};

			Entity();

			void name(const std::wstring & str);
			void value(const std::wstring & val);
			void notation(const std::wstring & str);
			void externalID(const ExternalID & extid);
			void reference(const Reference & ref);
			void type(EntityType t);

			std::wstring name(void) const;
			std::wstring value(void) const;
			std::wstring notation(void) const;
			ExternalID externalID(void) const;
			EntityType type(void) const;

		private:
			std::wstring	_name;
			ExternalID		_extId;
			std::wstring	_value;
			std::wstring	_notation;
			std::vector<Reference> _valrefs;
			EntityType		_type;
	};

}; // end namespace XML

#endif // INCLUDED_XML_ENTITY
