#ifndef INCLUDED_XML_ELEMENTDECL
#define INCLUDED_XML_ELEMENTDECL

#include <string>

namespace XML
{

	/*
	class xmlElementDecl
	{
		public:
			typedef enum { EMPTY, ANY, MIXED, CHILDREN } ContentSpec;

			XMLString		 	_name;
			ContentSpec			_type;
			list<XMLString>		_mixed;
			xmlElementContent	_content;
	};
	*/
	/*
	class xmlElemDecl
	{
		public:
			enum ContentModel { Empty, Any, Mixed, Children };

		//private:
			static xmlID		_idBase;
			xmlID				_elemId;
			ContentModel		_type;
			xmlElementContent	_children;
			list<xmlID>			_mixedContent;
	};
	*/

	class ElemDecl
	{
		public:
			typedef enum ContentSpec
			{
				Empty,
				Any,
				Mixed,
				Children
			} ContentSpec;

			void elementName(const std::wstring & elem);
			void contentspec(ContentSpec spec);

		private:
			std::wstring	_elementName; // name of the element
			ContentSpec		_contentspec;
	};

}; // end namespace XML

#endif // INCLUDED_XML_ELEMENTDECL
