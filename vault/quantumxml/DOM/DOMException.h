#ifndef INCLUDED_DOM_EXCEPTION
#define INCLUDED_DOM_EXCEPTION

namespace DOM
{

	class DOMException 
	{
		public:
			DOMException(unsigned short c);
			unsigned short code(void) const;

		private:
			unsigned short _code;
	};

	// ExceptionCode
	extern const unsigned short      INDEX_SIZE_ERR;
	extern const unsigned short      DOMSTRING_SIZE_ERR;
	extern const unsigned short      HIERARCHY_REQUEST_ERR;
	extern const unsigned short      WRONG_DOCUMENT_ERR;
	extern const unsigned short      INVALID_CHARACTER_ERR;
	extern const unsigned short      NO_DATA_ALLOWED_ERR;
	extern const unsigned short      NO_MODIFICATION_ALLOWED_ERR;
	extern const unsigned short      NOT_FOUND_ERR;
	extern const unsigned short      NOT_SUPPORTED_ERR;
	extern const unsigned short      INUSE_ATTRIBUTE_ERR;
	// Introduced in DOM Level 2:
	extern const unsigned short      INVALID_STATE_ERR;
	// Introduced in DOM Level 2:
	extern const unsigned short      SYNTAX_ERR;
	// Introduced in DOM Level 2:
	extern const unsigned short      INVALID_MODIFICATION_ERR;
	// Introduced in DOM Level 2:
	extern const unsigned short      NAMESPACE_ERR;
	// Introduced in DOM Level 2:
	extern const unsigned short      INVALID_ACCESS_ERR;

}; // end namespace DOM

#endif // INCLUDED_DOM_EXCEPTION
