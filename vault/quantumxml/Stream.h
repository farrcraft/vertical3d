#ifndef INCLUDED_XML_STREAM
#define INCLUDED_XML_STREAM

#include "XMLString.h"
#include "URL.h"

#include <iostream>

namespace XML
{

	// TODO:
	// languageID()
	// chars()

	/**
	 * Stream stores an in memory buffer. the stream can come from a
	 * network connection (using HTTP with a URL) or a file.
	 * it also handles any character encoding or translation. (in future versions)
	 **/
	class Stream
	{
		public:
			/**
			 * Default Constructor.
			 **/
			Stream();
			/**
			 * Construct a new stream with the given buffer.
			 * @param buf the buffer to initialize the stream with.
			 **/
			Stream(const std::wstring & buf);
			/**
			 * Default Destructor.
			 **/
			~Stream();

			/**
			 * Load the stream's buffer from the location referenced by the given URI.
			 * @param uri a URI formatted string.
			 * @return whether the buffer was successfully loaded.
			 **/
			bool			setURI(const std::string & uri);
			/**
			 * Get the URI formatted string representing the buffer's source.
			 * @return a URI formatted string.
			 **/
			std::string		getURI(void) const;
			/**
			 * Set the stream's internal character delimiter.
			 * @param delim the character used to delimit character sequences.
			 **/
			void 			setDelimiter(wchar_t delim);
			/**
			 * Get the stream's current internal character delimiter.
			 * @return the current internal character delimiter.
			 **/
			wchar_t			getDelimiter(void) const;
			/**
			 * Determine if the current character in the stream is some form of quote.
			 * @return 1 for double quote, 0 for single quote, -1 for neither.
			 **/
			int				isSingleDoubleQuote(void);

			// buffer manipulations

			/**
			 * Set the contents of the stream's internal buffer.
			 * @param buf the new stream buffer contents.
			 **/
			void			setBuffer(const std::wstring & buf);
			/**
			 * Move the current buffer pointer past any blank characters. The pointer
			 * is only moved if it is currently pointing at a blank character. After
			 * return, the current pointer is guaranteed not to point to a blank character.
			**/
			void			skipBlanks(void);
			/**
			 * Get the next character in the stream. The current pointer will be advanced
			 * by one character, excluding any newline sequences. 
			 * @return the next character in the stream.
			 **/
			wchar_t			nextChar(void);
			/**
			 * Skip over a sequence of characters in the stream.
			 * @param count the number of characters to skip over.
			 **/
			void			skipChars(unsigned long count);
			/**
			 * Look ahead from current stream position to char X in stream.
			 * @param pos the position in the stream to look at.
			 * @return the character at the specified position.
			 **/
			wchar_t			peek(unsigned long pos = 1);
			/**
			 * Mark the current stream position to make easy substr's. If there is an
			 * existing stream marker, then it will be replaced with the new one.
			 * @return the position in the stream that was marked.
			 **/
			unsigned long	mark(void);
			/**
			 * Set a stream marker for popping back to. If there is an existing push
			 * marker, then it will be replaced with the new one. This marker is separate
			 * from the one set by the mark() method.
			 **/
			void			push(void);
			/**
			 * Pop the stream back X chars. The current stream pointer will be reset to
			 * the position of the last push() call. If push was never called then the 
			 * new position will be set to the beginning of the buffer.
			 **/
			void			pop(void);
			/**
			 * Return the content of 'Chars*' or "Chars*".
			 * @return a literal character sequence.
			 **/
			std::wstring	literal(void);
			/** 
			 * Return the sequence of chars between sdelim and edelim
			 * @param sdelim the starting character delimiter
			 * @param edelim the end character delimiter to look for
			 * @return a sequence of characters.
			 **/
			std::wstring	chars(wchar_t sdelim, wchar_t edelim);

			/**
			 * Get the size of the stream's buffer.
			 * @return the current buffer size.
			 **/
			unsigned int	size(void) const;
			/**
			 * Determine if the current stream pointer has reached the end of the buffer.
			 * @return whether current pointer has reached the end of the buffer.
			 **/
			bool			end(void) const;

			// common XML productions

			/**
			 * Get a name character sequence from the buffer. The current pointer will be 
			 * advanced past the end of the name.
			 * @return a name string.
			 **/
			std::wstring	name(void);
			/**
			 * Get a nmtoken sequence from the buffer. The current pointer will be advanced
			 * past the end of the sequence.
			 * @return a nmtoken string.
			 **/
			std::wstring	nmtoken(void);
			/**
			 * Get a versionNum sequence from the buffer. The current pointer will be advanced
			 * past the end of the sequence.
			 * @return a versionNum string.
			 **/
			std::wstring	versionNum(void);
			/**
			 * Get a encName sequence from the buffer. The current pointer will be advanced
			 * past the end of the sequence.
			 * @return a encName string.
			 **/
			std::wstring	encName(void);
			/**
			 * Get a pubid literal sequence from the buffer. The current pointer will be advanced
			 * past the end of the sequence.
			 * @return a literal string.
			 **/
			std::wstring	pubidLiteral(void);
			/**
			 * Get a system literal sequence from the buffer. The current pointer will be advanced
			 * past the end of the sequence.
			 * @return a literal string.
			 **/
			std::wstring	systemLiteral(void);
			/**
			 * Get a language id sequence from the buffer. The current pointer will be advanced
			 * past the end of the sequence.
			 * @return a id string.
			 **/
			std::wstring	languageID(void);

			// namespace productions
			/**
			 * Get a namespace qualified attribute name character sequence from the buffer. 
			 * The current pointer will be advanced past the end of the name.
			 * @return a name string.
			 **/
			std::wstring	nsAttName(void);
			/**
			 * Get a name character sequence from the buffer. The current pointer will be 
			 * advanced past the end of the name.
			 * @return a name string.
			 **/
			std::wstring	ncName(void);
			/**
			 * Get a qualified name character sequence from the buffer. The current pointer 
			 * will be advanced past the end of the name.
			 * @return a name string.
			 **/
			std::wstring	qName(void);

			// these are just convenience wrappers for the defines

			/**
			 * Check if the current character pointed to in the stream is a blank.
			 * @return whether the next character is a blank.
			 **/
			bool			isBlank(void);
			/**
			 * Check if the next character pointed to in the stream is a normal character.
			 * @return whether the next character is a normal character.
			 **/
			bool			isChar(void);
			/**
			 * Check if the next character pointed to in the stream is a letter.
			 * @return whether the next character is a letter.
			 **/
			bool			isLetter(void);
			/**
			 * Check if the next character pointed to in the stream is an allowable name character.
			 * @return whether the next character is a name character.
			 **/
			bool			isNameChar(void);
			/**
			 * Check if the next character pointed to in the stream is a pubid character.
			 * @return whether the next character is a pubid character.
			 **/
			bool			isPubidChar(void);
			/**
			 * Check if the next character pointed to in the stream is a name character.
			 * @return whether the next character is a name character.
			 **/
			bool			isNcNameChar(void);

			// validation checks

			/**
			 * Determine if the given string is a valid name.
			 * @param str the string to check.
			 * @return whether the string is a valid name.
			 **/
			static bool		isName(const std::wstring & str);
			/**
			 * Determine if the given string is a valid sequence of blank delimited names.
			 * @param str the string to check.
			 * @return whether the string is a valid sequence of names.
			 **/
			static bool		isNames(const std::wstring & str);
			/**
			 * Determine if the given string is a valid nmtoken.
			 * @param str the string to check.
			 * @return whether the string is a valid nmtoken.
			 **/
			static bool		isNmtoken(const std::wstring & str);
			/**
			 * Determine if the given string is a valid sequence of blank delimited nmtokens.
			 * @param str the string to check.
			 * @return whether the string is a valid sequence of nmtokens.
			 **/
			static bool		isNmtokens(const std::wstring & str);

			// stream infos

			/**
			 * Get the current stream pointer position.
			 * @return position of the current stream pointer.
			 **/
			unsigned long	position(void);
			/**
			 * Get the current character pointed to in the stream.
			 * @return the current character in the stream.
			 **/
			wchar_t			current(void);
			/**
			 * Get the column position of the current stream pointer.
			 * @return the current column position.
			 **/
			unsigned long	column(void);
			/**
			 * Get the line number of the current stream pointer in the buffer.
			 * @return the current line number.
			 **/
			unsigned long	line(void);

			/**
			 * Compare the given string to the current buffer contents up to the specifed length.
			 * @param str the string to compare.
			 * @param length the number of characters in the buffer to compare.
			 * @return result of string/buffer comparison.
			 **/
			int				compare(const std::wstring & str, unsigned long length);
			/**
			 * Compare the given string to a substring within the stream's buffer.
			 * @param str the string to compare.
			 * @param start the offset from the current buffer pointer to begin comparing.
			 * @param length the number of characters in the buffer to compare.
			 * @return result of the string/buffer comparison.
			 **/
			int				compare(const std::wstring & str, unsigned long start, unsigned long length);
			/**
			 * Get a substring from the buffer beginning from the current buffer pointer.
			 * @param length the number of characters to copy.
			 * @return the requested substring.
			 **/
			std::wstring	substr(unsigned long length);
			/**
			 * Get a substring from the buffer beginning from an offset from the current buffer pointer.
			 * @param start the offset from the current buffer pointer to begin copying from.
			 * @param length the number of characters to copy.
			 * @return the requested substring.
			 **/
			std::wstring	substr(unsigned long start, unsigned long length);
			/**
			 * Get a substring from the buffer beginning at the position set by the last mark() call
			 * up to the current buffer pointer. If no position in the buffer has been previously
			 * marked then the beginning of the buffer is used.
			 * @return the requested substring.
			 **/
			std::wstring	substr(void);

			/**
			 * Get a character from the buffer.
			 * @param pos the position in the buffer to get the character from.
			 * @return the address of the character at the specified position.
			 **/
			const wchar_t	& operator[](unsigned long pos) const;

		private:
			unsigned long	_position;
			std::wstring	_buffer;
			unsigned long	_pushMark;
			unsigned long	_pushColumn;
			unsigned long	_pushLine;
			unsigned long	_mark;		// substr mark
			wchar_t			_current;
			wchar_t			_delimiter;
			unsigned long	_column;
			unsigned long	_line;
			URL				_uri;
	};

}; // namespace XML

#endif // INCLUDED_XML_STREAM
