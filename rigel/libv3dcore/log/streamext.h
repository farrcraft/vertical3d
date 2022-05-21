// ======================================================================================
// streamext.h - Version 1.1 - Copyright ©2002 Stephen W. Chappell
// Generic stream extensions to make my life easier
//
// Contents:
//   - operator<< for tm, based strongly on operator<< presented in "The Standard C++
//     Locale" by Nathan C. Myers, posted at html://www.cantrip.org/locale.html
//
// Revision History:
//   Date       | Ver | Author | Description
//   -----------+-----+--------+--------------------------------------------------------
//   01/11/2002 | 1.1 | SwC    | Problems with locale inclusion, so hdrs now always
//              |     |        |   included; also removed from slug namespace;
//              |     |        |   Microsoft compatibility; now outputs using specific
//              |     |        |   format ('c' doesn't work in MSVC, time doesn't output
//              |     |        |   readably)
//   10/09/2001 | 1.0 | SwC    | Initial Version
// ======================================================================================
#ifndef SLUG_STREAMEXT_H
#define SLUG_STREAMEXT_H

#include <ctime>
#include <locale>
#include <ostream>

// ======================================================================================
// Overloaded Operators
// ======================================================================================

// --------------------------------------------------------------------------------------
// operator<< - overloading for std::tm; allows outputting of the date using standard
//   streams; Microsoft's locale implementation is a little different from the standard
//   hence the different version, also time functions aren't in the std namespace in
//   MSVC
// --------------------------------------------------------------------------------------
#if !defined(_MSC_VER)
    template<class charT, class traits>
    std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT,traits>& os,
        const std::tm& date)
    {
        // typedefs to make the rest of this function more readable
        typedef std::ostreambuf_iterator<charT,traits> outIter_t;
        typedef std::time_put<charT, outIter_t> Facet;

        // makes this safe in a multithreaded environment
        typename std::basic_ostream<charT, traits>::sentry cerberus(os);
        if ( cerberus ) 
        {
            // output the date & time in the local format
            //const Facet& fac = std::use_facet<Facet>(os.getloc());
            //fac.put(os, os, os.fill(), &date, 'c');

            // output the date & time, plus weekday and AM/PM indicator, in the local 
            // format
            static const char format[] = "%a %x %X %p";
            const Facet& fac = std::use_facet<Facet>(os.getloc());
            if ( fac.put(os, os, os.fill(), &date, format, format + 11).failed() )
                os.setstate(os.badbit);
        }

        return os;
    }
#else
    template<class charT, class traits>
    std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT,traits>& os,
        const tm& date)
    {
        // typedefs to make the rest of this function more readable
        typedef std::ostreambuf_iterator<charT,traits> outIter_t;
        typedef std::time_put<charT, outIter_t> Facet;

        // makes this safe in a multithreaded environment
        std::basic_ostream<charT, traits>::sentry cerberus(os);
        if ( cerberus )
        {
            // output the date & time, plus weekday and AM/PM indicator, in the local 
            // format note that the simpler version of put (std version above) doesn't
            // work in MSVC
            static const char format[] = "%a %x %I:%M:%S %p";
            const Facet& fac = std::_USE(os.getloc(), Facet);
            if ( fac.put(os, os, &date, format, format + 17).failed() )
                os.setstate(os.badbit);
        }

        return os;
    }
#endif

// --------------------------------------------------------------------------------------
#endif
// ======================================================================================

