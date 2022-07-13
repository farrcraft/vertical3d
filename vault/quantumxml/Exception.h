#ifndef INCLUDED_XML_EXCEPTION
#define INCLUDED_XML_EXCEPTION

#include <string>

namespace XML
{

	/**
	 * An XML Parser Exception class.
	 **/
	class Exception
	{
		public:

			/**
			 * Parser Error Code enumeration
			 **/
			enum ErrorCode
			{
				UnknownScanError = 0,	/**< generic internal parser error */
				NoWhitespace,			/**< scanner expected whitespace */
				NoEq,					/**< scanner expected '=' */
				NoLiteral,				/**< scanner expected '"' or '\'' */
				LiteralValue,			/**< scanner expected a specific literal value */
				MismatchedLiteral,		/**< scanner expected '"' but got '\'' (or v.v.) */
				ExpectedChar,			/**< expected char (in message) */
				IllegalChar,			/**< scanner found illegal char (see message for context/char) */
				WFC,					/**< WFC Check error */
				StreamError,			/**< parser stream error */
			};

			/**
			 * Exception constructor.
			 * @param c the error code of this exception
			 **/
			Exception(ErrorCode c);

			/**
			 * Return a string description of the exception's error code.
			 * @return an error code description
			 **/
			std::string error() const;
			/**
			 * Return a context-sensitive error message
			 * @return the error message
			 **/
			std::string message() const;
			/**
			 * Set a context-sensitive error message.
			 * @param msg the error message to be set
			 **/
			void message(const std::string & msg);
			/**
			 * Get the exception code
			 * @return an error code
			 */
			unsigned short code() const;

		private:
			unsigned short	code_;
			std::string		message_;
	};

}; // namespace XML

#endif // INCLUDED_XML_EXCEPTION

