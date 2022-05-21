#ifndef INCLUDED_XML_ATTDEF
#define INCLUDED_XML_ATTDEF

#include <vector>
#include <string>
#include <sstream>

namespace XML
{

	class AttDef
	{
		public:
			enum AttType	 
			{ 
				CData, 
				Id, 
				IdRef, 
				IdRefs, 
				Entity, 
				Entities, 
				Nmtoken, 
				Nmtokens, 
				Enumerated, 
				Notation 
			};
			enum DefaultType
			{ 
				Required, 
				Implied, 
				Fixed, 
				Value 
			};

			AttDef(const std::wstring & name);

			void type(AttType t);
			void defaultType(DefaultType d);
			void enumeration(const std::wstring & name); 
			void value(const std::wstring & val);

		private:
			AttType			_type;
			DefaultType		_default;
			std::wstring	_name;
			std::vector<std::wstring> _enumeration;
			std::wstring	_value;
	};

}; // end namespace XML

#endif // INCLUDED_XML_ATTDEF
