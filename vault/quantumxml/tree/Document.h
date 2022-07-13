#ifndef INCLUDED_XML_DOCUMENT
#define INCLUDED_XML_DOCUMENT

#include "Doctype.h"
#include "Misc.h"
#include "Element.h"

#include "../URL.h"
#include "../DOM/Document.h"

namespace XML
{

	class Document : public DOM::Document
	{
		public:
			Document();
			~Document();

			enum Flags
			{
				// about the parsed document
				NoXMLDecl			= (1<<1), 
				NoEncodingDecl		= (1<<2), 
				NoSDDecl			= (1<<3),
				NoDocTypeDecl		= (1<<4)
			};

			void version(const std::wstring & vers);
			void standalone(const std::wstring & sa);
			void encoding(const std::wstring & enc);

			boost::shared_ptr<Misc> misc (void);
			void addMisc(boost::shared_ptr<Misc> m);

			bool sync(const URL & uri);

			unsigned int flags(void);
			void flags(unsigned int f);
			void setFlag(unsigned int f);

			void setDoctype(const std::wstring & name);
			void setRootElement(boost::shared_ptr<Element> elem);

		private:
			boost::shared_ptr<Misc> _misc;
			unsigned int _flags;
	};

}; // end namespace XML

#endif // INCLUDED_XML_DOCUMENT
