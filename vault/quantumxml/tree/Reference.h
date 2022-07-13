#ifndef INCLUDED_XML_REFERENCE
#define INCLUDED_XML_REFERENCE

#include <string>

#include "Node.h"

namespace XML
{

	class Reference : public Node
	{
		public:
			enum RefType
			{
				PERef,
				CharRef,
				EntityRef,
				None
			};

			Reference();

			void type(RefType t);
			void value(const std::wstring & val);

			RefType type(void) const;
			std::wstring value(void) const;

		private:
			RefType _type;
			std::wstring _value;
	};

}; // end namespace XML

#endif // INCLUDED_XML_REFERENCE
