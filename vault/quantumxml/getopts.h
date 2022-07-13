#ifndef INCLUDED_GETOPTS
#define INCLUDED_GETOPTS

#include <string>
#include <vector>

/*
things that aren't currently implemented/supported:
- catching unrecognized options
- arguments without a separating space after a shortopt e.g "-o<arg>"
- smart checking of - or -- opt prefixes to distinguish between short and long opts
- printing out supported opts and args
- code documentation
- working out options in the order they're given (precedence/overriding)
- multiple arguments per option
*/
class GetOpts
{
	public:
		/**
		 * option string format: "shortopt|longopt*:shortopt**:"
		 **/
		GetOpts(int argc, char * argv[], const std::string & opts);
		~GetOpts();

		/**
		 * get next shortopt
		 *
		 **/
		int next(void);
		/**
		 * get argument for current option
		 *
		 **/
		std::string getarg(void);

	protected:
		class Option
		{
			public:
				std::string _arg;
				bool _needarg;
				bool _optionalarg;
				char _shortopt;
				bool _used;
				std::string _longopt;
		};

		/**
		 * parse arguments 
		 * precondition: must parse options first
		 *
		 **/
		void parse_args(int argc, char * argv[]);
		/**
		 * lookup an option name and return whether it is recognized or not
		 *
		 **/
		bool match_opt(const std::string & opt);
		/**
		 * parse options string into option structures
		 *
		 **/
		void parse_opts(const std::string & opts);

	private:
		std::vector<std::string> _args;
		std::vector<Option> _opts;
		unsigned int _position;
};

#endif // INCLUDED_GETOPTS
