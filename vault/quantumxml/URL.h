#ifndef INCLUDED_XML_URL
#define INCLUDED_XML_URL

#include <string>

namespace XML
{

	/**
	 * A Uniform Resource Locator class.
	 * This class takes a string representing a URL and parses it into the
	 * component pieces that make up the URL.
	 **/
	class URL
	{
		public:
			/**
			 * Default Constructor.
			 **/
			URL();
			/**
			 * Copy Constuctor.
			 **/
			URL(const URL & url);
			/**
			 * Create a new URL from a string representation.
			 * @param url the url formatted string to parse.
			 **/
			URL(const std::string & url);
			/**
			 * Default Destructor.
			 **/
			~URL();

			/**
			 * Assignment operator.
			 * @param url url formatted string to parse.
			 * @return a reference to the URL object.
			 **/
			URL & operator = (const std::string & url);

			/**
			 * Get the full url as a string.
			 * @return the url formatted string.
			 **/
			std::string getURL(void) const;
			/**
			 * Get the scheme portion of the url. Examples of valid schemes are: 
			 * http, ftp, file, etc.
			 * @return a string containing the scheme name.
			 **/
			std::string getScheme(void) const;
			/**
			 * Get the url params.
			 * @return a string containing the url params.
			 **/
			std::string getParams(void) const;
			/**
			 * Get the network location portion of the url.
			 * @return a string containing the url's network location.
			 **/
			std::string getNetLocation(void) const;
			/**
			 * Get the query portion of the url.
			 * @return the url's query string.
			 **/
			std::string getQuery(void) const;
			/**
			 * Get the path portion of the url.
			 * @return the url's path string.
			 **/
			std::string getPath(void) const;
			/**
			 * Get the host portion of the url's network location.
			 * @return the host name.
			 **/
			std::string getHost(void) const;
			/**
			 * Get the port number portion of the url's network location.
			 * @return the url's port number, or -1 if none is specified.
			 **/
			short  getPort(void) const;

		protected:
			bool parse(void);

		private:
			std::string _url;
			std::string _scheme;
			std::string _params;
			std::string _query;
			std::string _path;
			std::string _netLocation;
	};

}; // end namespace XML

#endif // INCLUDED_XML_URL
