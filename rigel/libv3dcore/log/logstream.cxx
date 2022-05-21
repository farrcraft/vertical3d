// ======================================================================================
// logstream.cpp - Version 1.0 - Copyright ©2002 Stephen W. Chappell
// A customized output stream for logging data
//
// Namespace: slug
//
// Requires: streamext.h
//
// Contents:
//   - logbuf    - a filtering streambuf used with the logstream; it inserts the date,
//                 time, and log level into the real buffer
//   - log_base  - a base class for the logstream; it contains members needed to
//                 initialize the other logstream base, ostream, as well as a workaround
//                 for not being able to cast rdbuf() is MSVC
//   - logstream - an ostream descendent that uses the filtering logbuf to output data
//
// Description:
//   This customized ostream descendant extends the ostream class using standard (but
//   somewhat cryptic) methods. The logstream itself is simply an ostream descendant that
//   utilizes a custom streambuf. The custom streambuf (logbuf, a streambuf descendant)
//   is what really does the work:
//
//   - the logbuf filters the data going to some other streambuf or streambuf descendant,
//     essentially looking for newlines so it can insert some stuff in the beginning
//   - the other streambuf does its normal thing to output data; it's supplied to the
//     logbuf constructor as a parameter
//
//   This might seem complicated, but since the streambuf is what really does all the
//   work in standard iostreams, this makes a whole lot of sense, and, now that it's
//   done, it seems a whole lot less complicated
//
//   For the moment, log_base contains a std::filebuf to use to actually output the data,
//   at some point it would be nice to make this a parameter so that, for example, some
//   sort of remote window logging or multithreaded logging could be implemented with
//   another custom streambuf (the logbuf itself shouldn't have to change)
//
// Additional credits:
//   - the basic class architecture was strongly influenced by the iostreams postings of
//     Dietmar Kühl regarding "prefix" streams at
//     http://www.inf.uni-konstanz.de/~kuehl/c++/iostream/
//
// Revision History:
//   Date       | Ver | Author | Description
//   -----------+-----+--------+--------------------------------------------------------
//   01/14/2002 | 1.0 | SwC    | Initial Version
// ======================================================================================
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <streambuf>
#include <string>
#include <sstream>

#include "streamext.h"
#include "logstream.h"
// --------------------------------------------------------------------------------------
using std::filebuf;
using std::endl;
using std::setw;
using std::ios;
using std::ios_base;
using std::locale;
using std::ostream;
using std::streambuf;
using std::string;
using std::stringstream;

#ifndef _MSC_VER
    // MSVC doesn't include these in the std namespace
    using std::time_t;
    using std::time;
    using std::localtime;
#endif

// ======================================================================================

namespace slug
{

// ======================================================================================
// Function Implementation
// ======================================================================================

// --------------------------------------------------------------------------------------
// operator<< for const char* - overloaded << operator for logstreams and const chars;
//   this is needed because the template creates ambiguities with existing ostream
//   operators
// --------------------------------------------------------------------------------------
logstream& operator<<(logstream &lhs, const char *rhs)
{
    std::ostream::sentry cerberus(lhs);
    if ( cerberus )
    {
        if ( lhs.get_logbuf()->level() & lhs.filter() )
            *(dynamic_cast<std::ostream*>(&lhs)) << rhs;
    }
	lhs.flush();
    return lhs;
}

// --------------------------------------------------------------------------------------
// operator<< for setlevel manipulator - sets the current log level
// --------------------------------------------------------------------------------------
logstream& operator<<(logstream &lhs, const setlevel &rhs)
{
    std::ostream::sentry cerberus(lhs);
    if ( cerberus )
    {
        lhs.level(rhs.m_level);
    }
	lhs.flush();
    return lhs;
}

// ======================================================================================
// logbufImpl Declaration
// ======================================================================================

struct logbufImpl
{
    streambuf    *m_sbuf;     // the actual streambuf used to read and write chars
    bool          m_bol,      // remember whether we are at a new line
                  m_suppress; // suppress the prefix output when set
    unsigned      m_level;
    stringstream  m_datebuf;  // used to construct the date; this is a member so it
                              // doesn't have to be imbued with the locale more than once

    explicit logbufImpl(streambuf *sb)
        :m_sbuf(sb),
         m_bol(true),
         m_suppress(false),
         m_level(log_base::ll_info),
         m_datebuf(ios_base::out) {}

    const char* leveltext(void);
};

// ======================================================================================
// logbuf Implementation
// ======================================================================================

// --------------------------------------------------------------------------------------
// logbuf::logbuf - constructor
// --------------------------------------------------------------------------------------
logbuf::logbuf(streambuf *sb)
    :impl( new logbufImpl(sb) )
{
    setp(0, 0); // initializes the put buffer to be empty so overflow gets called
    impl->m_datebuf.imbue( locale("") );

    // I don't know if this is just a bug in the Borland compiler or what, but for some
    // reason the date that comes out in m_datebuf the first time it gets written to is
    // not correct (11/30/1990 on 1/9/2002)
    time_t now = time(0);
    impl->m_datebuf.str("");
    impl->m_datebuf << *localtime(&now);
}

// --------------------------------------------------------------------------------------
// logbuf::~logbuf - destructor
// --------------------------------------------------------------------------------------
logbuf::~logbuf()
{
    if ( !impl->m_bol )
    {
        impl->m_suppress = true;
        overflow('\n');
    }

    delete impl;
}

// --------------------------------------------------------------------------------------
// logbuf::level - sets the level of the next line to be output
// --------------------------------------------------------------------------------------
void logbuf::level(unsigned level)
{
    impl->m_level = level;

    // if the level is changed and we're not at the end of a line, then we need to
    // advance to the next line
    if ( !impl->m_bol )
    {
        overflow('\n');
    }
}

// --------------------------------------------------------------------------------------
// logbuf::level - gets the level of the next line to be output
// --------------------------------------------------------------------------------------
unsigned logbuf::level(void) const
{
    return impl->m_level;
}

// --------------------------------------------------------------------------------------
// logbuf::suppress - suppresses the output of the line prefix
// --------------------------------------------------------------------------------------
void logbuf::suppress(bool value)
{
    impl->m_suppress = value;
}

// --------------------------------------------------------------------------------------
// logbuf::suppress - suppresses the output of the line prefix
// --------------------------------------------------------------------------------------
bool logbuf::suppress(void) const
{
    return impl->m_suppress;
}

// --------------------------------------------------------------------------------------
// logbuf::logbuf - overflow is called when the put buffer is full; since the put buffer
//   is size 0 for this logbuf, it gets called for every char; normally this is forwarded
//   to the "real" streambuf, but in the case of a newline we set m_bol, and if m_bol is
//   already set the line prefix is output
// --------------------------------------------------------------------------------------
int logbuf::overflow(int ch)
{
    if ( ch != EOF )
    {
        // when we get to the beginning of a line, output the date & time, and the
        // currently set message level
        if ( impl->m_bol & !impl->m_suppress )
        {
            // get the date and save it off
            time_t now = time(0);
            impl->m_datebuf.str("");
            impl->m_datebuf << *localtime(&now) << " [" << impl->leveltext() << "] ";
            string nowstring = impl->m_datebuf.str();

            // output the date, etc
            if ( impl->m_sbuf->sputn(nowstring.c_str(), nowstring.size())
                 != static_cast<signed>(nowstring.size()) )
                return EOF;
            else
                impl->m_bol = false;
        }

        int rc = impl->m_sbuf->sputc(ch);

        if ( ch == '\n' )
            impl->m_bol = true;

        return rc;
    }

    return 0;
}

// --------------------------------------------------------------------------------------
// logbuf::sync - syncs the (nonexistent) buffers with the actual output
// --------------------------------------------------------------------------------------
int logbuf::sync()
{
    impl->m_sbuf->pubsync();
    return 0;
}

// ======================================================================================
// logbuf Implementation
// ======================================================================================

// --------------------------------------------------------------------------------------
// logbufImpl::leveltext - outputs the best match name for the currently selected level
// --------------------------------------------------------------------------------------
const char* logbufImpl::leveltext(void)
{
    if      ( m_level & log_base::ll_debug   ) return "DEBUG  ";
    else if ( m_level & log_base::ll_info    ) return "INFO   ";
    else if ( m_level & log_base::ll_warning ) return "WARNING";
    else if ( m_level & log_base::ll_error   ) return "ERROR  ";
    else if ( m_level & log_base::ll_trace   ) return "TRACE  ";
    else if ( m_level & log_base::ll_winmsg  ) return "WINMSG ";

    return "UNKNOWN";
}

// ======================================================================================
// logstream Implementation
// ======================================================================================

// --------------------------------------------------------------------------------------
// logstream::logstream - constructor; constructs log_base base, then uses it's m_buf
//   member to construct the ostream base; this is important - without this arrangement
//   something else would have to be done like maintaining a dynamic filebuf
// --------------------------------------------------------------------------------------
logstream::logstream()
    :log_base(),
     ostream( m_logbuf ),
     m_filter(log_base::ll_error | log_base::ll_warning)
{
}

// --------------------------------------------------------------------------------------
// logstream::~logstream - destructor
// --------------------------------------------------------------------------------------
logstream::~logstream()
{
}

// --------------------------------------------------------------------------------------
// logstream::open - open the requested file in append mode
// --------------------------------------------------------------------------------------
void logstream::open(const char *filename)
{
    // open the file
    m_buf.open(filename, ios::out | ios::app);

    // output a divider; this is cast to ostream to avoid filtering
    bool old_suppress = get_logbuf()->suppress();
    get_logbuf()->suppress(true);

    setf(ios::left);
    width(115);
    char old_fill = fill('=');
    *dynamic_cast<ostream*>(this) << ">" << "<\n";
    fill(old_fill);

    get_logbuf()->suppress(old_suppress);
}

// --------------------------------------------------------------------------------------
// logstream::level - sets the level for the next data to be output
// --------------------------------------------------------------------------------------
logstream& logstream::level(unsigned value)
{
    get_logbuf()->level(value);
	flush();
	return *this;
}

// --------------------------------------------------------------------------------------
// logstream::write_hex - writes hex data to the log
// --------------------------------------------------------------------------------------
void logstream::write_hex(const char *p_data, unsigned size, unsigned level)
{
    // if this message isn't of the right level, discard it
    if ( !(level & m_filter) ) return;
    get_logbuf()->level(level);

    // output message header
    *dynamic_cast<ostream*>(this) << "Hex dump (" << size << " bytes):\n\t";

    // output the first hex address
    char orig_fill = fill('0');
    setf(ios::right);
    *dynamic_cast<ostream*>(this) << setw(8) << std::hex << 0x00000000 << ' ';
    setf(ios::left);
    fill(orig_fill);

    const unsigned char *data = reinterpret_cast<const unsigned char*>(p_data),
                        *line = reinterpret_cast<const unsigned char*>(p_data);

    // output data in rows of hex, with a break every 16 bytes, and ASCII translation at
    // the end of the line
    for ( unsigned pos = 0 ; pos < size ; ++pos, ++data )
    {
        if ( pos )
        {
            if ( !(pos % 16) ) // end of line; add the translation & go to next line
            {
                // translate the line of text
                *dynamic_cast<ostream*>(this) << "  ";
                translate(line, data);
                *dynamic_cast<ostream*>(this) << "\n\t";
                line = data;

                // output the next line's hex address
                fill('0');
                setf(ios::right);
                *dynamic_cast<ostream*>(this) << std::hex << setw(8) << pos << ' ';
                setf(ios::left);
                fill(orig_fill);
            }
            else if ( !(pos % 8) ) // middle of line; insert an extra space
            {
                *dynamic_cast<ostream*>(this) << ' ';
            }
        }

        fill('0');
        *dynamic_cast<ostream*>(this) << std::hex << setw(2) << static_cast<short>(*data) << ' ';
        fill(orig_fill);
    }

    // we hit the end of the data in the middle somewhere, so space over to where the
    // translation should appear
    size %= 16;
    if ( size )
    {
        if ( size < 8 ) *dynamic_cast<ostream*>(this) << ' ';
        for ( unsigned i = 16 - size ; i != 0 ; --i )
            *dynamic_cast<ostream*>(this) << setw(3) << ' ';
    }

    // output the translation of the last (incomplete) line of data
    *dynamic_cast<ostream*>(this) << "  ";
    translate(line, data);
    *dynamic_cast<ostream*>(this) << std::endl;
}

// --------------------------------------------------------------------------------------
// logstream::translate - translates characters from start to end into ASCII text
// --------------------------------------------------------------------------------------
void logstream::translate(const unsigned char *start, const unsigned char *end)
{
    for ( ; start != end ; ++start )
    {
        get_logbuf()->sputc(( *start > 32 ) ? *start : '.');
    }
}

// ======================================================================================

} // namespace

// ======================================================================================

