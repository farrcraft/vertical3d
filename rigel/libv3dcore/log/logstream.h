// ======================================================================================
// logstream.h - Version 1.0 - Copyright ©2002 Stephen W. Chappell
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
#ifndef SLUG_LOGSTREAM_H
#define SLUG_LOGSTREAM_H

#include <fstream>
#include <ostream>
#include <streambuf>

// ======================================================================================

namespace slug
{

// ======================================================================================
// Class Declarations
// ======================================================================================

// --------------------------------------------------------------------------------------
// class logbuf - this is a "filtering" streambuf; it filters data destined for the
//   output, when it finds a newline it inserts the log line header
// Note that the streambuf passed in the constructor is NOT owned by logbuf
// --------------------------------------------------------------------------------------
class logbuf : public std::streambuf
{
public:
    explicit logbuf(std::streambuf *sb);
    virtual ~logbuf();
    
    void     level(unsigned level);
    unsigned level(void) const;
        // setter/getter for the level of the next line to be output

    void suppress(bool value);
    bool suppress(void) const;
        // suppresses the output of the line prefix

protected:
    int overflow(int ch);
        // called to output the next character

    int sync();
        // called to sync the buffer with the actual output

private:
    class logbufImpl *impl; // hides implementation to reduce dependencies
};

// --------------------------------------------------------------------------------------
// class log_base - exists solely for the filebuf & logbuf members to be inherited, so
//   that they're available to initialize logstream's ostream base with
// --------------------------------------------------------------------------------------
class log_base
{
public:
    enum log_level // LogMask - values control what messages get logged
    {
        ll_debug   = 0x0001,
        ll_info    = 0x0002,
        ll_warning = 0x0004,
        ll_error   = 0x0008,
        ll_trace   = 0x0010,
        ll_winmsg  = 0x0020,
        ll_all     = 0xFFFF
    };

    log_base():m_buf(), m_logbuf( new logbuf(&m_buf) ) {}
    virtual ~log_base() { delete m_logbuf; }

    logbuf* get_logbuf(void) const  { return m_logbuf;  }
        // returns a pointer to the log output buffer
        // this is a concession for MSVC - trying to cast rdbuf() results in AV's, so
        // this is a less than perfect workaround

protected:
    std::filebuf  m_buf;    // this must be declared before m_logbuf!
    logbuf       *m_logbuf; 
};

// --------------------------------------------------------------------------------------
// class logstream - an output stream tailored to logging
// --------------------------------------------------------------------------------------
class logstream : public log_base, public std::ostream
{
public:
    logstream();
    ~logstream();

    void open(const char *filename);

    logstream& level(unsigned value);
        // sets the level for the next data to be output

    void     filter(unsigned value) { m_filter = value; }
    unsigned filter(void) const     { return m_filter;  }
        // sets/gets the filter for the next data to be output

    void write_hex(const char *p_data, unsigned size, unsigned level);

private:
    unsigned  m_filter;

    void translate(const unsigned char *start, const unsigned char *end);
};

// --------------------------------------------------------------------------------------
// class setlevel - an output manipulator for the logstream
// --------------------------------------------------------------------------------------
struct setlevel
{
    setlevel(unsigned level):m_level(level) {}
    unsigned m_level;
};

// ======================================================================================
// Function Declarations
// ======================================================================================

// --------------------------------------------------------------------------------------
// operator<< - overloaded for logstream to allow filtering; doesn't work as a class
//   member so it's implemented as a standalone (manipulators like endl become ambiguous)
// Additional overloads for custom manipulators and to resolve the odd ambiguity (e.g.
//   const char*) 
// --------------------------------------------------------------------------------------
template <typename T>
logstream& operator<<(logstream &lhs, const T& rhs)
{
    std::ostream::sentry cerberus(lhs);
    if ( cerberus )
    {
        if ( lhs.get_logbuf()->level() & lhs.filter() )
            *(dynamic_cast<std::ostream*>(&lhs)) << rhs;
    }

    return lhs;
}

logstream& operator<<(logstream &lhs, const char *rhs);
logstream& operator<<(logstream &lhs, const setlevel &rhs);

} // namespace slug

// --------------------------------------------------------------------------------------
#endif
// ======================================================================================

