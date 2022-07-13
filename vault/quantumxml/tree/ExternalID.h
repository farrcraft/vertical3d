#ifndef INCLUDED_XML_EXTERNALID
#define INCLUDED_XML_EXTERNALID

#include <iostream>
#include <fstream>

namespace XML
{

	// used in dtd
	class ExternalID
	{
		public:
			void publicId(const std::wstring & pid);
			void systemId(const std::wstring & sid);

			std::wstring publicId(void) const;
			std::wstring systemId(void) const;

			bool sync(std::wofstream & xml_file);

		private:
			std::wstring _publicId;
			std::wstring _systemId;
	};

}; // end namespace XML

#endif // INCLUDED_XML_EXTERNALID
