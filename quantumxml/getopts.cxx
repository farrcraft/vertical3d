#include "getopts.h"

#include <iostream>

GetOpts::GetOpts(int argc, char * argv[], const std::string & opts)
{
	// initialize options list
	parse_opts(opts);
	// extract used options and arguments
	parse_args(argc, argv);
	// set initial option position
	_position = 0;
}

GetOpts::~GetOpts()
{
}

// get the next used shortopt name
int GetOpts::next(void)
{
	// skip over unused options while there are some left in the list
	while (_position < _opts.size() && !_opts[_position]._used)
		_position++;
	// reached end of options list
	if (_position == _opts.size())
		return EOF;
	// return next used short opt name
	return _opts[_position++]._shortopt;
}

// get argument for current option
std::string GetOpts::getarg(void)
{
	return _opts[_position-1]._arg;
}

// parse command line options and arguments
void GetOpts::parse_args(int argc, char * argv[])
{
	bool gotopt;
	// go over each option or argument given
	for (int i = 1; i < argc; i++)
	{
		std::string this_arg = argv[i];

		gotopt = false;
		// go over all of the recognized options in the list
		for (unsigned int j = 0; j < _opts.size(); j++)
		{
			// does this look like a shortopt?
			if (strlen(argv[i]) == 2 && argv[i][0] == '-')
			{
				// does it match the current shortopt in the list?
				if (_opts[j]._shortopt == argv[i][1])
				{
					// mark the option as having been used
					_opts[j]._used = true;
					// found an option so check for matching arguments
					gotopt = true;
				}
			}
			else if (_opts[j]._longopt == this_arg.substr(2)) // is this a recognized longopt name?
			{
				// mark the option as having been used
				_opts[j]._used = true;
				// found an option so check for matching arguments
				gotopt = true;
			}
			if (gotopt)
			{
				// does this option require an argument and is there at least one left to parse?
				if (_opts[j]._needarg && !_opts[j]._optionalarg && (i+1 != argc))
				{
					// save the option's argument
					_opts[j]._arg = argv[++i];
				}
				else if (_opts[j]._needarg && _opts[j]._optionalarg && (i+1 != argc))
				{
					// is this really an optional argument or just the next option?
					if (!match_opt(argv[i+1]))
					{
						// save the option's argument
						_opts[j]._arg = argv[++i];
					}
				}
				// found an option list match so move on to the next one
				break;
			}
		}
	}
}

// look for a option name and return whether recognized or not
bool GetOpts::match_opt(const std::string & opt)
{
	// go through the entire options list
	for (unsigned int j = 0; j < _opts.size(); j++)
	{
		// does it look like a shortopt?
		if (opt.size() == 2 && opt[1] == '-')
		{
			// does it match?
			if (_opts[j]._shortopt == opt[0])
			{
				return true;
			}
		}
		else
		{
			// does it match this longopt name?
			if (_opts[j]._longopt.compare(opt.substr(2)))
			{
				return true;
			}
		}
	}
	// no recognized option matches
	return false;
}

// build the options list structures from the given option string
// option string format: "shortopt|longopt*:shortopt**:"
// eg: "w|wfc:v|validate:s|silent:f|file*:o|outfile*"
void GetOpts::parse_opts(const std::string & opts)
{
	// position in option string of next option separator (if any)
	std::string::size_type next_opt = opts.find(":");
	// starting position in option string of this option
	std::string::size_type this_opt = 0;
	// loop conditional
	std::string::size_type pos = 0;
	// iterate over the option string while there is still something left to parse
	while (pos < opts.size())
	{
		// initialize a new option structure
		Option opt;
		opt._shortopt = opts[this_opt];
		opt._optionalarg = false;
		opt._needarg = false;
		opt._used = false;
		// look for a longopt name separator character for this option (if any)
		std::string::size_type long_div = opts.rfind("|", next_opt);
		// does this option take an optional argument?
		if (opts[next_opt - 2] == '*' && opts[next_opt - 1] == '*')
		{
			opt._optionalarg = true;
			opt._needarg = true;
		}
		else if (opts[next_opt - 1] == '*')
		{
			// this option takes a required argument
			opt._needarg = true;
		}
		if (long_div != std::string::npos)
		{
			// extract the longopt name
			std::string::size_type offset = long_div + 1;
			if (opt._needarg)
				offset++;
			if (opt._optionalarg)
				offset++;
			opt._longopt = opts.substr(long_div + 1, next_opt - offset);
		}
		// add the option to the list
		_opts.push_back(opt);
		// move pointers to the next starting position in the option string
		this_opt = ++next_opt;
		next_opt = opts.find(":", next_opt);
		// the last option won't be followed by an option separator ':' so compensate in that case
		if (next_opt == std::string::npos && pos < opts.size() - 1)
		{
			next_opt = opts.size();
			pos = opts.size() - 1;
		}
		else
			pos = next_opt;
	};
}
