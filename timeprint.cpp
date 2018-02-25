/*******************************************************************************
This program prints the current date and time to the standard output stream.
It takes an optional format string to control the output.
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

using std::wstring;
using std::vector;
using std::wcout;


enum class HelpType    // Types of usage information for the --help option
{
    None,         // No help information requested
    General,      // General usage information
    Examples,     // Illustrative examples
    DeltaTime,    // Delta time formats
    FormatCodes,  // Output format time codes
    TimeSyntax,   // ISO 8601 explicit time format
    TimeZone      // Time zone formats
};


enum class TimeType    // Type of time for an associated time value string
{
    None,         // Not a legal time
    Now,          // Current time
    Explicit,     // Explicit ISO-8601 date/time
    Access,       // Access time of the named file
    Creation,     // Creation time of the named file
    Modification  // Modification time of the named file
};


struct TimeSpec
{
    TimeType type;     // Type of time
    wstring  value;    // String value of specified type
};


struct Parameters
{
    // Describes the parameters for a run of this program.

    wchar_t  codeChar;      // Format Code Character (default '%')
    HelpType helpType;      // Type of help information to print & exit
    wstring  zone;          // Time zone string
    wstring  format;        // Output format string

    TimeSpec time1;         // Time 1 [required] (either single use, or for time difference)
    TimeSpec time2;         // Time 2 [optional] (for time difference output)
};


// Global Variables
static time_t timeNow;


// Function Declarations
bool calcTime            (const Parameters& params, struct tm& timeValue, time_t& deltaTimeSeconds);
bool getParameters       (Parameters& params, int argc, wchar_t* argv[]);
bool getTime             (time_t& result, const TimeSpec&);
bool getExplicitDateTime (time_t& result, wstring timeSpec);
bool getExplicitTime     (struct tm& result, wstring::iterator specBegin, wstring::iterator specEnd);
bool getExplicitDate     (struct tm& result, wstring::iterator specBegin, wstring::iterator specEnd);
void help                (HelpType);
void printResults        (wstring format, wchar_t codeChar, const struct tm& timeValue, time_t deltaTimeSeconds);
void printDelta          (wstring::iterator& formatIterator, const wstring::iterator& formatEnd, time_t deltaTimeSeconds);
bool printDeltaFunc      (wstring::iterator& formatIterator, const wstring::iterator& formatEnd, time_t deltaTimeSeconds);


//__________________________________________________________________________________________________
int wmain (int argc, wchar_t *argv[])
{
    time (&timeNow);    // Snapshot the current time.

    Parameters params;

    if (!getParameters(params, argc, argv)) return -1;

    help (params.helpType);

    struct tm currentTime;
    time_t    deltaTimeSeconds;

    if (calcTime (params, currentTime, deltaTimeSeconds)) {
        printResults (params.format, params.codeChar, currentTime, deltaTimeSeconds);
        return 0;
    }

    return 1;
}


//__________________________________________________________________________________________________
bool getParameters (Parameters &params, int argc, wchar_t* argv[])
{
    // This function processes the command line arguments and sets the corresponding values in the
    // Parameters structure. This function returns true if all arguments were legal and processed
    // properly, otherwise it returns false.

    // Set default values.
    params.codeChar = L'%';
    params.helpType   = HelpType::None;
    params.time1.type = TimeType::None;
    params.time2.type = TimeType::None;

    // Process command arguments.
    for (auto i=1;  i < argc;  ++i) {
        auto argptr = argv[i];

        if (!((argv[i][0] == L'-') || (argv[i][0] == L'/'))) {
            if (params.format.empty())
                params.format += argptr;
            else {
                params.format += L" ";
                params.format += argptr;
            }
        } else {
            auto timeValWord1 = i;         // Used later for reporting on invalid third time values.
            auto timeValWord2 = false;

            auto optChar = argv[i][1];     // Option Character

            if (optChar == 0) {
                fputws (L"timeprint: Null option switch.\n", stderr);
                return false;
            }

            // Point argptr to the contents of the switch.  This may be immediately following the
            // option character, or it may be the next token on the command line.

            auto     advanceArg = (argv[i][2] == 0);
            wchar_t* switchWord = nullptr;

            if (optChar == L'-') {
                advanceArg = true;
                switchWord = argv[i] + 2;
                if (0 == _wcsicmp(switchWord, L"access"))
                    optChar = L'a';
                else if (0 == _wcsicmp(switchWord, L"codeChar"))
                    optChar = L'%';
                else if (0 == _wcsicmp(switchWord, L"creation"))
                    optChar = L'c';
                else if (0 == _wcsicmp(switchWord, L"help"))
                    optChar = L'h';
                else if (0 == _wcsicmp(switchWord, L"modification"))
                    optChar = L'm';
                else if (0 == _wcsicmp(switchWord, L"now")) {
                    optChar = L'n';
                    advanceArg = false;
                }
                else if (0 == _wcsicmp(switchWord, L"time"))
                    optChar = L't';
                else if (0 == _wcsicmp(switchWord, L"timeZone"))
                    optChar = L'z';
                else {
                    fwprintf (stderr, L"timeprint: Unrecognized switch (--%s).\n", switchWord);
                    return false;
                }
            }

            if (advanceArg) {
                ++i;
                if (i >= argc) {
                    argptr = 0;
                } else {
                    argptr = argv[i];
                    timeValWord2 = true;
                }
            } else {
                argptr = argv[i]+2;
            }

            // Handle the option according to the option character.

            TimeType newTimeType = TimeType::None;    // For options that specify a time

            switch (optChar) {
                default:
                    fwprintf (stderr, L"timeprint: Unrecognized option (-%c).\n", optChar);
                    return false;

                // File Access Time
                case L'a':
                    if (!argptr) {
                        fwprintf (stderr, L"timeprint: Missing argument for --access (-a) option.\n");
                        return false;
                    }
                    newTimeType = TimeType::Access;
                    break;

                // File Modification Time
                case L'c':
                    if (!argptr) {
                        fwprintf (stderr, L"timeprint: Missing argument for --creation (-c) option.\n");
                        return false;
                    }
                    newTimeType = TimeType::Creation;
                    break;

                // Alternate Code Character
                case L'%':
                    if (argptr) params.codeChar = *argptr;
                    break;

                // Command Usage & Help
                case L'H':
                case L'h':
                case L'?':
                    if (!argptr) {
                        params.helpType = HelpType::General;
                    } else if (0 == _wcsicmp(argptr, L"examples")) {
                        params.helpType = HelpType::Examples;
                    } else if (0 == _wcsicmp(argptr, L"deltaTime")) {
                        params.helpType = HelpType::DeltaTime;
                    } else if (0 == _wcsicmp(argptr, L"formatCodes")) {
                        params.helpType = HelpType::FormatCodes;
                    } else if (0 == _wcsicmp(argptr, L"timeSyntax")) {
                        params.helpType = HelpType::TimeSyntax;
                    } else if (0 == _wcsicmp(argptr, L"timeZone")) {
                        params.helpType = HelpType::TimeZone;
                    } else {
                        params.helpType = HelpType::General;
                    }
                    return true;

                // File Modification Time
                case L'm':
                    if (!argptr) {
                        fwprintf (stderr, L"timeprint: Missing argument for --modification (-m) option.\n");
                        return false;
                    }
                    newTimeType = TimeType::Modification;
                    break;

                case L'n':
                    newTimeType = TimeType::Now;
                    break;

                case L't':
                    if (!argptr) {
                        fwprintf (stderr, L"timeprint: Missing argument for --time (-t) option.\n");
                        return false;
                    }
                    newTimeType = TimeType::Explicit;
                    break;

                // Timezone
                case L'z':
                    if (!argptr) {
                        fwprintf (stderr, L"timeprint: Missing argument for --timeZone (-z) option.\n");
                        return false;
                    }

                    params.zone = argptr;
                    break;
            }

            if (newTimeType != TimeType::None) {
                if (params.time1.type == TimeType::None) {
                    params.time1.type = newTimeType;
                    if (argptr) params.time1.value = argptr;
                } else if (params.time2.type == TimeType::None) {
                    params.time2.type = newTimeType;
                    if (argptr) params.time2.value = argptr;
                } else {
                    fwprintf (stderr,
                        L"timeprint: Unexpected third time value (%s%s%s).\n",
                        argv[timeValWord1],
                        timeValWord2 ? L" " : L"",
                        timeValWord2 ? argv[timeValWord1 + 1] : L"");
                    return false;
                }
            }
        }
    }

    // If no format string was specified on the command line, fetch it from the TIMEFORMAT
    // environment variable.  If not available there, then use the default format string.

    if (params.format.empty()) {
        wchar_t* timeFormat;
        _wdupenv_s (&timeFormat, nullptr, L"TIMEFORMAT");

        if (!timeFormat) {
            params.format = L"%#c";
        } else {
            params.format = timeFormat;
            free (timeFormat);
        }
    }

    // If no time source was specified, then report information for the current time.
    if (params.time1.type == TimeType::None)
        params.time1.type = TimeType::Now;

    return true;
}


//__________________________________________________________________________________________________
bool calcTime (
    const Parameters& params,            // Command parameters
    struct tm&        timeValue,         // Output time value
    time_t&           deltaTimeSeconds)  // Output time delta in seconds
{
    // This function computes the time results and then sets the timeValue and deltaTimeSeconds
    // parameters. This function returns true on success, false on failure.

    // If an alternate time zone was specified, then we need to set the TZ environment variable.
    // Kludgey, but I couldn't find another way.

    if (!params.zone.empty()) {
        _wputenv_s (L"TZ", params.zone.c_str());
    }

    time_t time1;
    if (!getTime (time1, params.time1)) return false;

    if (params.time2.type == TimeType::None) {      // Reporting a single absolute time.
        deltaTimeSeconds = 0;
        localtime_s (&timeValue, &time1);
    } else {                                        // Reporting a time diffence
        time_t time2;
        if (!getTime (time2, params.time2)) return false;
        deltaTimeSeconds = (time1 < time2) ? (time2 - time1) : (time1 - time2);
        gmtime_s (&timeValue, &deltaTimeSeconds);
    }

    return true;
}


//__________________________________________________________________________________________________
bool getTime (time_t& result, const TimeSpec& spec)
{
    // Gets the time according to the spec's type + value.

    if (spec.type == TimeType::Now) {
        result = timeNow;
        return true;
    }

    if (  (spec.type == TimeType::Access)
       || (spec.type == TimeType::Creation)
       || (spec.type == TimeType::Modification)) {

        struct _stat stat;    // File Status Data

        auto fileName = spec.value.c_str();

        if (0 != _wstat(fileName, &stat)) {
            fwprintf (stderr, L"timeprint: Couldn't get status of \"%s\".\n", fileName);
            return false;
        }

        switch (spec.type) {
            case TimeType::Access:        result = stat.st_atime; break;
            case TimeType::Creation:      result = stat.st_ctime; break;
            case TimeType::Modification:  result = stat.st_mtime; break;
        }
        return true;
    }

    if (spec.type == TimeType::Explicit) {
        auto timeString = spec.value;
        if (getExplicitDateTime(result, timeString))
            return true;

        fwprintf (stderr, L"timeprint: Unrecognized explicit time: \"%s\".\n", timeString.c_str());
        return false;
    }

    return false;   // Unrecognized time type
}


/*==================================================================================================
Temporary Notes: Legal ISO-8601 Formats for timeprint

Syntax
    Done   Syntax                            Missing Values
    -----  --------------------------------  -------------------------------------------------------
    [ ]    <timeValue> = <date>              time = current time
    [ ]    <timeValue> = <time>              date = current date
    [ ]    <timeValue> = <date>T<time>

    [ ]    <date> = <YYYY>-?<MM>-?<DD>
    [ ]    <date> = <YYYY>                   month = current month, day = current day
    [ ]    <date> = <YYYY>-<MM>              day = current day
    [ ]    <date> = --<MM>-?<DD>             year = current year
    [ ]    <date> = <YYYY>-?<DDD>

    [ ]    <YYYY> = 0000-9999
    [ ]    <MM>   = 01-12
    [ ]    <DD>   = 01-31
    [ ]    <DDD>  = 001-366

    [ ]    <time> = <HH>:?<MM>:?<SS><zone>
    [ ]    <time> = <HH>:?<MM><zone>         seconds = current seconds
    [ ]    <time> = <HH><zone>               minutes = current minutes, seconds = current seconds

    [ ]    -?     = - | <null>
    [ ]    :?     = : | <null>
    [ ]    +-     = + | -
    [ ]    <HH>   = 00-24                    24 = midnight / 00 of next day
    [ ]    <MM>   = 00-59
    [ ]    <SS>   = 00-60                    Accomodates leap seconds

    [ ]    <zone> = <null>                   use current local time zone
    [ ]    <zone> = Z
    [ ]    <zone> = [+-]<HH>:?<MM>
    [ ]    <zone> = [+-]<HH>

    [ ]  6 --####       --MMDD
    [ ]  7 ####-##      YYYY-MM
    [ ]  7 --##-##      --MM-DD
    [ ]  7 #######      YYYYDDD
    [ ]  8 ########     YYYYMMDD
    [ ]  8 ####-###     YYYY-DDD
    [ ] 10 ####-##-##   YYYY-MM-DD

    [x]  2 ##           hh
    [x]  4 ####         hhmm
    [x]  5 ##:##        hh:mm
    [x]  6 ######       hhmmss
    [x]  8 ##:##:##     hh:mm:ss

    [x]         ...#Z   +0000
    [x]       ...#-##   -HH00
    [x]     ...#-####   -hhmm
    [x]    ...#-##:##   -hhmm
    [x]       ...#+##   +hh00
    [x]     ...#+####   +hhmm
    [x]    ...#+##:##   +hhmm

    No T:
        try time
        try date

    No T:
        has :|+|Z   = time
        multiple -  = date
        end -####   = time
        len 7,8     = date
        else        = time

==================================================================================================*/

/*
struct tm
{
    int tm_sec;   // seconds after the minute - [0, 60] including leap second
    int tm_min;   // minutes after the hour - [0, 59]
    int tm_hour;  // hours since midnight - [0, 23]
    int tm_mday;  // day of the month - [1, 31]
    int tm_mon;   // months since January - [0, 11]
    int tm_year;  // years since 1900
    int tm_wday;  // days since Sunday - [0, 6]
    int tm_yday;  // days since January 1 - [0, 365]
    int tm_isdst; // daylight savings time flag
};
*/

bool getExplicitDateTime (time_t& result, wstring timeSpec)
{
    struct tm timeStruct;
    gmtime_s (&timeStruct, &timeNow);

    auto dateTimeSep = std::find (timeSpec.begin(), timeSpec.end(), 'T');
    bool successResult;

    if (dateTimeSep != timeSpec.end()) {
        successResult = getExplicitTime (timeStruct, std::next(dateTimeSep), timeSpec.end())
                     && getExplicitDate (timeStruct, timeSpec.begin(), dateTimeSep);
    } else {
        successResult = getExplicitTime (timeStruct, timeSpec.begin(), timeSpec.end())
                     || getExplicitDate (timeStruct, timeSpec.begin(), timeSpec.end());
    }

    if (successResult)
        result = mktime (&timeStruct);

    return successResult;
}


bool parseDateTimePatternCore (
    wchar_t*           pattern,
    wstring::iterator& sourceIt,
    wstring::iterator  sourceEnd,
    vector<int>&       results)
{
    results.clear();

    auto sourceStart = sourceIt;
    auto numberValue = 0;
    auto capturingNumber = false;

    for (auto patternChar=pattern;  *patternChar;  ++patternChar, ++sourceIt) {

        if (sourceIt == sourceEnd) return false;

        if (capturingNumber && (*patternChar != '#')) {
            results.push_back (numberValue);
            numberValue = 0;
            capturingNumber = false;
        }

        switch (*patternChar) {
            // Captured elements
            case L'#': {
                if (!isdigit(*sourceIt)) return false;
                numberValue = (10 * numberValue) + (*sourceIt - '0');
                capturingNumber = true;
                break;
            }

            case L'+': {
                if (*sourceIt == L'-')
                    results.push_back (-1);
                else if (*sourceIt == L'+')
                    results.push_back (1);
                else
                    return false;
                break;
            }

            // Non-captured elements
            case L'-': {
                if (*sourceIt != L'-') --sourceIt;
                break;
            }

            case L'=': {
                if (*sourceIt != L'-') return false;
                break;
            }

            case L':': {
                if (*sourceIt != L':') --sourceIt;
                break;
            }

            default: {
                if (*sourceIt != *patternChar) return false;
                break;
            }
        }
    }

    if (capturingNumber)
        results.push_back (numberValue);

    return true;
}


bool parseDateTimePattern (
    wchar_t*           pattern,
    wstring::iterator& sourceIt,
    wstring::iterator  sourceEnd,
    vector<int>&       results)
{
    wstring::iterator sourceReset = sourceIt;

    if (!parseDateTimePatternCore (pattern, sourceIt, sourceEnd, results)) {
        sourceIt = sourceReset;
        return false;
    }

    return true;
}


/*
    [ ]    <time> = <HH>:?<MM>:?<SS><zone>
    [ ]    <time> = <HH>:?<MM><zone>         seconds = current seconds
    [ ]    <time> = <HH><zone>               minutes = current minutes, seconds = current seconds
    [ ]    -?     = - | <null>
    [ ]    :?     = : | <null>
    [ ]    +-     = + | -
    [ ]    <HH>   = 00-24                    24 = midnight / 00 of next day
    [ ]    <MM>   = 00-59
    [ ]    <SS>   = 00-60                    Accomodates leap seconds
    [ ]    <zone> = <null>                   use current local time zone
    [ ]    <zone> = Z
    [ ]    <zone> = [+-]<HH>:?<MM>
    [ ]    <zone> = [+-]<HH>
    [ ]  2 ##           hh          ##
    [ ]  4 ####         hhmm        ##:##
    [ ]  5 ##:##        hh:mm       ##:##
    [ ]  6 ######       hhmmss      ##:##:##
    [ ]  8 ##:##:##     hh:mm:ss    ##:##:##
    [ ]         ...#Z   +0000       Z
    [ ]       ...#-##   -HH00       +##
    [ ]     ...#-####   -hhmm       +##:##
    [ ]    ...#-##:##   -hhmm       +##:##
    [ ]       ...#+##   +hh00       +##
    [ ]     ...#+####   +hhmm       +##:##
    [ ]    ...#+##:##   +hhmm       +##:##
*/

bool getExplicitTime (struct tm& resultTime, wstring::iterator specBegin, wstring::iterator specEnd)
{
    bool gotTime = false;
    vector<int> results;
    wstring::iterator specIt = specBegin;

    if (parseDateTimePattern (L"##:##:##", specIt, specEnd, results)) {
        resultTime.tm_hour = results[0];
        resultTime.tm_min  = results[1];
        resultTime.tm_sec  = results[2];
        gotTime = true;
    } else if (parseDateTimePattern (L"##:##", specIt, specEnd, results)) {
        resultTime.tm_hour = results[0];
        resultTime.tm_min  = results[1];
        gotTime = true;
    } else if (parseDateTimePattern (L"##", specIt, specEnd, results)) {
        resultTime.tm_hour = results[0];
        gotTime = true;
    }

    if (!gotTime) return false;

    // Parse timezone, if any.

    if ((specIt == specEnd) || ((specIt[0] == L'Z') && (std::next(specIt) == specEnd))) {
        // UTC time, no adjustment needed.
        return true;
    }

    if (parseDateTimePattern (L"+##:##", specIt, specEnd, results)) {
        resultTime.tm_hour += results[0] * results[1];
        resultTime.tm_min  += results[0] * results[1];
    } else if (parseDateTimePattern (L"+##", specIt, specEnd, results)) {
        resultTime.tm_hour += results[0] * results[1];
    }

    return (specIt == specEnd);
}


/*
    [ ]    <timeValue> = <date>              time = current time
    [ ]    <timeValue> = <time>              date = current date
    [ ]    <timeValue> = <date>T<time>
    [ ]    <date> = <YYYY>-?<MM>-?<DD>
    [ ]    <date> = <YYYY>                   month = current month, day = current day
    [ ]    <date> = <YYYY>-<MM>              day = current day
    [ ]    <date> = --<MM>-?<DD>             year = current year
    [ ]    <date> = <YYYY>-?<DDD>
    [ ]    <YYYY> = 0000-9999
    [ ]    <MM>   = 01-12
    [ ]    <DD>   = 01-31
    [ ]    <DDD>  = 001-366
    [ ]  6 --####       --MMDD        ==##-##
    [ ]  7 ####-##      YYYY-MM       ####=##
    [ ]  7 --##-##      --MM-DD       ==##-##
    [ ]  7 #######      YYYYDDD       ####-###
    [ ]  8 ########     YYYYMMDD      ####-##-##
    [ ]  8 ####-###     YYYY-DDD      ####-###
    [ ] 10 ####-##-##   YYYY-MM-DD    ####-##-##
*/
bool getExplicitDate (struct tm& resultTime, wstring::iterator specBegin, wstring::iterator specEnd)
{
    auto gotDate = false;
    vector<int> results;
    wstring::iterator specIt = specBegin;

    if (parseDateTimePattern (L"==##-##", specIt, specEnd, results)) {
        resultTime.tm_mon  = results[0] - 1;
        resultTime.tm_mday = results[1];
        gotDate = true;
    } else if (parseDateTimePattern (L"####-##-##", specIt, specEnd, results)) {
        resultTime.tm_year = results[0] - 1900;
        resultTime.tm_mon  = results[1] - 1;
        resultTime.tm_mday = results[2];
        gotDate = true;
    } else if (parseDateTimePattern (L"####-###", specIt, specEnd, results)) {
        resultTime.tm_year = results[0] - 1900;
        resultTime.tm_mon  = 0;
        resultTime.tm_mday = results[1];
        gotDate = true;
    } else if (parseDateTimePattern (L"####=##", specIt, specEnd, results)) {
        resultTime.tm_year = results[0] - 1900;
        resultTime.tm_mon  = results[1] - 1;
        gotDate = true;
    } else if (parseDateTimePattern (L"####", specIt, specEnd, results)) {
        resultTime.tm_year = results[0] - 1900;
        gotDate = true;
    }

    return gotDate && (specIt == specEnd);
}


//__________________________________________________________________________________________________
void printResults (
    wstring          format,             // The format string, possibly with escape sequences and format codes
    wchar_t          codeChar,           // The format code character (normally %)
    const struct tm& timeValue,          // The primary time value to use
    time_t           deltaTimeSeconds)   // Time difference when comparing two times
{
    // This procedure scans through the format string, emitting expanded codes and escape sequences
    // along the way.

    auto formatEnd = format.end();

    for (auto formatIterator = format.begin();  formatIterator != formatEnd;  ++formatIterator) {
        const auto buffsize = 1024;
        wchar_t    buff [buffsize];        // Intermediate Output Buffer

        // Handle backslash sequences, unless backslash is the alternate escape character.

        if ((*formatIterator == L'\\') && (codeChar != L'\\')) {
            ++formatIterator;

            switch (*formatIterator) {

                // If the string ends with a \, then just emit the \.
                case 0:     putwchar(L'\\');  break;

                // Recognized \-sequences are handled here.
                case L'n':  putwchar(L'\n');  break;
                case L't':  putwchar(L'\t');  break;
                case L'b':  putwchar(L'\b');  break;
                case L'r':  putwchar(L'\r');  break;
                case L'a':  putwchar(L'\a');  break;

                // Unrecognized \-sequences resolve to the escaped character.
                default:
                    putwchar(*formatIterator);
                    break;
            }

        } else if (*formatIterator == codeChar) {

            const static auto legalCodes = L"%aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ";

            wchar_t token[4];    // Code Token Word

            ++formatIterator;

            if (*formatIterator == L'_') {
                printDelta (++formatIterator, formatEnd, deltaTimeSeconds);
                --formatIterator;  // Reset iterator for loop increment.
            } else if ((*formatIterator != L'#') && !wcschr(legalCodes, *formatIterator)) {
                // Print out illegal codes as-is.
                putwchar (L'%');
                putwchar (*formatIterator);
            } else if ((formatIterator[0] == L'#') && !wcschr(legalCodes, formatIterator[1])) {
                // Print out illegal '#'-prefixed codes as-is.
                ++formatIterator;
                putwchar (L'%');
                putwchar (L'#');
                putwchar (*formatIterator);
            } else {
                // Standard legal strftime() Code Sequences
                token[0] = L'%';
                token[1] = *formatIterator;
                token[2] = 0;

                if (*formatIterator == L'#') {
                    token[2] = *++formatIterator;
                    token[3] = 0;
                }

                wcsftime (buff, sizeof(buff), token, &timeValue);
                fputws (buff, stdout);
            }

        } else {
            // All unescaped characters are emitted as-is.
            if (*formatIterator)
                putwchar (*formatIterator);
        }
    }

    putwchar (L'\n');
}


/*==================================================================================================
Delta Formatting
==================================================================================================*/

const double secondsPerMinute       = 60;
const double secondsPerHour         = secondsPerMinute * 60;
const double secondsPerDay          = secondsPerHour * 24;
const double secondsPerNominalYear  = secondsPerDay * 365;
const double secondsPerTropicalYear = secondsPerDay * (365.0 + 97.0/400.0);


void printDelta (
    wstring::iterator&       formatIterator,     // Pointer to delta format after '%_'
    const wstring::iterator& formatEnd,          // Format string end
    time_t                   deltaTimeSeconds)   // Time difference when comparing two times
{
    // This function attempts to print the time delta format. If the format is bad, then print the
    // format substring as-is and restore the format string pointer to continue.

    auto formatRestart = formatIterator;

    if (!printDeltaFunc(formatIterator, formatEnd, deltaTimeSeconds)) {
        fputws (L"%_", stdout);
        formatIterator = formatRestart;
    }
}


bool charIn (wchar_t c, const wchar_t* list)
{
    // Return true if the given character is in the zero-terminated array of characters.
    // Also returns true if c == 0.
    auto i = 0;
    while (list[i] && (c != list[i]))
        ++i;
    return (c == list[i]);
}


bool printDeltaFunc (
    wstring::iterator&       formatIterator,     // Pointer to delta format after '%_'
    const wstring::iterator& formatEnd,          // Format string end
    time_t                   deltaTimeSeconds)   // Time difference when comparing two times
{
    double  deltaValue;              // Delta value, scaled
    wchar_t thousandsChar = 0;       // Thousands-separator character, 0=none
    wchar_t decimalChar = 0;         // Decimal character

    if (formatIterator == formatEnd) return false;

    // Parse thousands separator and decimal point format flag.
    if (*formatIterator == L'\'') {
        if (++formatIterator == formatEnd) return false;
        thousandsChar = *formatIterator;
        if (++formatIterator == formatEnd) return false;
        decimalChar = *formatIterator;
        if (++formatIterator == formatEnd) return false;

        if (thousandsChar == L'0')
            thousandsChar = 0;
    }

    // Parse modulo unit, if one exists.
    wchar_t moduloUnit  = *formatIterator++;
    double  moduloValue = 0;

    switch (moduloUnit) {
        case L'y':  moduloValue = secondsPerNominalYear;   break;
        case L't':  moduloValue = secondsPerTropicalYear;  break;
        case L'd':  moduloValue = secondsPerDay;           break;
        case L'h':  moduloValue = secondsPerHour;          break;
        case L'm':  moduloValue = secondsPerMinute;        break;

        default:
            moduloUnit = 0;
            --formatIterator;
            break;
    }

    deltaValue = moduloUnit ? fmod(deltaTimeSeconds,moduloValue) : deltaTimeSeconds;

    // Parse delta unit.

    if (formatIterator == formatEnd) return false;

    auto unitType = *formatIterator++;

    switch (unitType) {
        case L'Y': {
            if (moduloUnit != 0) return false; // There are no legal modulo unit prefixes for year.
            deltaValue /= secondsPerNominalYear;
            break;
        }

        case L'T': {
            if (moduloUnit != 0) return false; // There are no legal modulo unit prefixes for year.
            deltaValue /= secondsPerTropicalYear;
            break;
        }

        case L'D': {
            if (!charIn(moduloUnit, L"ty")) return false; // Filter out invalid modulo unit prefixes.
            deltaValue /= secondsPerDay;
            break;
        }

        case L'H': {
            if (!charIn(moduloUnit, L"tyd")) return false; // Filter out invalid modulo unit prefixes.
            deltaValue /= secondsPerHour;
            break;
        }

        case L'M': {
            if (!charIn(moduloUnit, L"tydh")) return false; // Filter out invalid modulo unit prefixes.
            deltaValue /= secondsPerMinute;
            break;
        }

        case L'S': {
            if (!charIn(moduloUnit, L"tydhm")) return false; // Filter out invalid modulo unit prefixes.
            break;
        }

        default: return false;
    }

    // Determine the precision of the output value.

    std::wostringstream output;            // Number value string
    auto               precision = 0;     // Output decimal precision

    if (unitType == L'S') {
        // Seconds have no fractional value.
    } else if ((formatIterator == formatEnd) || (*formatIterator != L'.')) {
        deltaValue = floor(deltaValue);
    } else {
        ++formatIterator;
        if ((formatIterator == formatEnd ) || !isdigit(*formatIterator)) {
            switch (unitType) {
                case L'T':
                case L'Y': precision = 8; break;
                case L'D': precision = 5; break;
                case L'H': precision = 4; break;
                case L'M': precision = 2; break;
            }
        } else {
            while ((formatIterator != formatEnd) && isdigit(*formatIterator))
                precision = 10*precision + (*formatIterator++ - L'0');
        }
    }

    // Get the string value of the deltaValue with the requested precision.
    output << std::fixed << std::setprecision(precision) << deltaValue;
    wstring outputString = output.str();

    auto decimalPointIndex = outputString.rfind(L'.');

    // Replace decimal point if requested.
    if (decimalChar && (decimalPointIndex != wstring::npos))
        outputString.replace(decimalPointIndex, 1, 1, decimalChar);

    // Insert thousands separator character if requested.
    if (thousandsChar) {
        auto kGroupIndex = 0;

        if (decimalPointIndex == wstring::npos)
            kGroupIndex = static_cast<int>(outputString.length() - 3);
        else {
            kGroupIndex = static_cast<int>(decimalPointIndex - 3);
        }
        
        while (kGroupIndex > 0) {
            outputString.insert (kGroupIndex, 1, thousandsChar);
            kGroupIndex -= 3;
        }
    }

    fputws (outputString.c_str(), stdout);
    return true;
}


//__________________________________________________________________________________________________
static auto help_general =
    L"timeprint v2.0.0-beta  |  https://github.com/hollasch/timeprint\n"
    L"timeprint - Print time and date information\n"
    L"\n"
    L"usage: timeprint [--codeChar <char>] [-%<char>]\n"
    L"                 [--help [topic]] [-h[topic]] [/?]\n"
    L"                 [--access <fileName>] [-a<fileName>]\n"
    L"                 [--creation <fileName>] [-c<fileName>]\n"
    L"                 [--modification <fileName>] [-m<fileName>]\n"
    L"                 [--timeZone <zone>] [-z<zone>]\n"
    L"                 [--now] [-n]\n"
    L"                 [--time <timeValue>] [-t<timeValue>]\n"
    L"                 [string] ... [string]\n"
    L"\n"
    L"This command prints time information to the standard output stream. All string\n"
    L"fragments will be concatenated with a space, so it's usually unnecessary to\n"
    L"quote the format string.\n"
    L"\n"
    L"timeprint operates in either absolute or differential mode. If one time value\n"
    L"is specified, then values for that absolute time are reported. If two time\n"
    L"values are supplied, then timeprint reports the values for the positive\n"
    L"difference between those two values. If no time values are given, then --now\n"
    L"is implied.\n"
    L"\n"
    L"Command switches may be prefixed with a dash (-) or a slash (/).\n"
    L"\n"
    L"    --access <fileName>, -a<fileName>\n"
    L"        Use the time of last access of the named file for a time value.\n"
    L"\n"
    L"    --codeChar <char>, -%<char>\n"
    L"        The --codeChar switch specifies an alternate code character to the\n"
    L"        default '%' character. If the backslash (\\) is specified as the code\n"
    L"        character, then normal backslash escapes will be disabled. The\n"
    L"        --codeChar switch is ignored unless the format string is specified on\n"
    L"        the command line.\n"
    L"\n"
    L"    --creation <fileName>, -c <fileName>\n"
    L"        Use the creation time of the named file.\n"
    L"\n"
    L"    --help [topic], -h[topic], -?[topic]\n"
    L"        Print help and usage information in general, or for the specified\n"
    L"        topic. Topics include 'examples', 'deltaTime', 'formatCodes',\n"
    L"        'timeSyntax', and 'timezone'.\n"
    L"\n"
    L"    --modification <fileName>, -m<fileName>\n"
    L"        Use the modification time of the named file.\n"
    L"\n"
    L"    --now, -n\n"
    L"        Use the current time.\n"
    L"\n"
    L"    --time <value>, -t<value>\n"
    L"        Specifies an explicit absolute time, using ISO 8601 syntax. For a\n"
    L"        description of supported syntax, use '--help timeSyntax'.\n"
    L"\n"
    L"    --timeZone <zone>, -z<zone>\n"
    L"        The --timeZone argument takes a timezone string of the form used by\n"
    L"        the _tzset function. If no timezone is specified, the system local\n"
    L"        time is used. The timezone can be set in the environment via the TZ\n"
    L"        environment variable. For a description of the time zone format, use\n"
    L"        '--help timeZone'.\n"
    L"\n"
    L"If no output string is supplied, the format specified in the environment\n"
    L"variable TIMEFORMAT is used. If this variable is not set, then the format\n"
    L"defaults to \"%#c\".\n"
    L"\n"
    L"Note that if your format string begins with - or /, you will need to prefix it\n"
    L"with a \\ character so that it is not confused with a command switch.\n"
    L"\n"
    L"Strings take both \\-escaped characters and %-codes in the style of printf.\n"
    L"The escape codes include \\n (newline), \\t (tab), \\b (backspace),\n"
    L"\\r (carriage return), and \\a (alert, or beep).\n"
    L"\n"
    L"For a full description of supported time format codes, use\n"
    L"'--help formatCodes'.\n"
    L"\n"
    L"For additional help, use '--help <topic>', where <topic> is one of:\n"
    L"    - examples\n"
    L"    - deltaTime\n"
    L"    - formatCodes\n"
    L"    - timeSyntax\n"
    L"    - timeZone\n"
    ;

static auto help_formatCodes =
    L"\n"
    L"    The following time format codes are supported:\n"
    L"\n"
    L"    %a     Abbreviated weekday name *\n"
    L"    %A     Full weekday name *\n"
    L"    %b     Abbreviated month name *\n"
    L"    %B     Full month name *\n"
    L"    %c     Date and time representation *\n"
    L"    %C     Year divided by 100 and truncated to integer (00-99)\n"
    L"    %d     Day of month as decimal number (01-31)\n"
    L"    %D     Short MM/DD/YY date, equivalent to %m/%d/%y\n"
    L"    %e     Day of the month, space-padded ( 1-31)\n"
    L"    %F     Short YYYY-MM-DD date, equivalent to %Y-%m-%d\n"
    L"    %g     Week-based year, last two digits (00-99)\n"
    L"    %G     Week-based year\n"
    L"    %h     Abbreviated month name (same as %b) *\n"
    L"    %H     Hour in 24-hour format (00-23)\n"
    L"    %I     Hour in 12-hour format (01-12)\n"
    L"    %j     Day of year as decimal number (001-366)\n"
    L"    %m     Month as decimal number (01-12)\n"
    L"    %M     Minute as decimal number (00-59)\n"
    L"    %n     New line character (same as '\\n')\n"
    L"    %p     AM or PM designation\n"
    L"    %r     12-hour clock time *\n"
    L"    %R     24-hour HH:MM time, equivalent to %H:%M\n"
    L"    %S     Seconds as a decimal number (00-59)\n"
    L"    %t     Horizontal tab character (same as '\\t')\n"
    L"    %T     ISO 8601 time format (HH:MM:SS) equivalent to %H:%M:%S\n"
    L"    %u     ISO 8601 weekday as number with Monday=1 (1-7)\n"
    L"    %U     Week number, first Sunday = week 1 day 1 (00-53)\n"
    L"    %V     ISO 8601 week number (01-53)\n"
    L"    %w     Weekday as decimal number, Sunday = 0 (0-6)\n"
    L"    %W     Week of year, decimal, Monday = week 1 day 1(00-51)\n"
    L"    %x     Date representation *\n"
    L"    %X     Time representation *\n"
    L"    %y     Year without century, as decimal number (00-99)\n"
    L"    %Y     Year with century, as decimal number\n"
    L"    %z     ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100)\n"
    L"           If timezone cannot be determined, no characters\n"
    L"    %Z     Time-zone name or abbreviation, empty for unrecognized zones *\n"
    L"    %_...  Delta time formats. See `--help deltaTime`.\n"
    L"    %%     Percent sign\n"
    L"\n"
    L"    * Specifiers marked with an asterisk are locale-dependent.\n"
    L"\n"
    L"As in the printf function, the # flag may prefix any formatting code. In that\n"
    L"case, the meaning of the format code is changed as follows.\n"
    L"\n"
    L"    %#c\n"
    L"        Long date and time representation, appropriate for current locale.\n"
    L"        For example: Tuesday, March 14, 1995, 12:41:29.\n"
    L"\n"
    L"    %#x\n"
    L"        Long date representation, appropriate to current locale.\n"
    L"        For example: Tuesday, March 14, 1995.\n"
    L"\n"
    L"    %#d, %#H, %#I, %#j, %#m, %#M, %#S, %#U, %#w, %#W, %#y, %#Y\n"
    L"        Remove any leading zeros.\n"
    L"\n"
    L"    All others\n"
    L"        The flag is ignored.\n"
    ;

static auto help_deltaTime =
    L"    Delta Time Formatting\n"
    L"\n"
    L"    Time differences are reported using the delta time formats. The delta time\n"
    L"    format has the following syntax:\n"
    L"\n"
    L"                               %_['kd][p]<u>[.[#]]\n"
    L"                                  -v-  v  v  --v-\n"
    L"            Numeric Format --------'   |  |    |\n"
    L"            Next Greater Unit ---------'  |    |\n"
    L"            Units ------------------------'    |\n"
    L"            Decimal Precision -----------------'\n"
    L"\n"
    L"    Numeric Format ['kd] (optional)\n"
    L"        The optional ' character is followed by two characters, k and d.\n"
    L"        k represents the character to use for the thousand's separator, with\n"
    L"        the special case that '0' indicates that there is to be no thousands\n"
    L"        separator. The d character is the character to use for the decimal\n"
    L"        point, if one is present. So, for example, \"'0.\" specifies no\n"
    L"        thousands separator, and the American '.' decimal point. \"'.,\" would\n"
    L"        specify European formatting, with '.' for the thousands separator, and\n"
    L"        ',' as the decimal point.\n"
    L"\n"
    L"    Next Greater Unit [p] (optional)\n"
    L"        This single lowercase letter indicates any preceding units used in the\n"
    L"        delta time printing. For example, if the unit is hours, and the next\n"
    L"        greater unit is years, then the hours reported are the remainder\n"
    L"        (modulo) after the number of years. Supported next greater units\n"
    L"        include the following:\n"
    L"\n"
    L"            y - Nominal years (see units below for definition)\n"
    L"            t - Tropical years (see units below for definition)\n"
    L"            d - Days\n"
    L"            h - Hours\n"
    L"            m - Minutes\n"
    L"\n"
    L"    Units <u> (required)\n"
    L"        The unit of time (single uppercase letter) to report for the time\n"
    L"        delta. This is the remainder after the (optional) next greater unit.\n"
    L"        The following units are supported:\n"
    L"\n"
    L"            Y - Nominal years\n"
    L"            T - Tropical years\n"
    L"            D - Days\n"
    L"            H - Hours\n"
    L"            M - Minutes\n"
    L"            S - Whole seconds\n"
    L"\n"
    L"        Nominal years are 365 days in length.\n"
    L"\n"
    L"        Tropical (or solar) years are approximately equal to one trip around\n"
    L"        the sun. These are useful to approximate the effect of leap years when\n"
    L"        reporting multi-year durations.\n"
    L"\n"
    L"        The following are the supported combinations of next greater unit and\n"
    L"        unit:\n"
    L"\n"
    L"            Y\n"
    L"            T\n"
    L"            D yD tD\n"
    L"            H yH tH dH\n"
    L"            M yM tM dM hM\n"
    L"            S yS tS dS hS mS\n"
    L"\n"
    L"    Decimal Precision [.[#]] (optional)\n"
    L"        With the exception of seconds, all units will have a fractional value\n"
    L"        for time differences. If the decimal precision format is omitted, the\n"
    L"        then rounded whole value is printed.\n"
    L"        \n"
    L"        If the decimal point and number is specified, then the fractional\n"
    L"        value will be printed with the number of requested digits.\n"
    L"\n"
    L"        If a decimal point is specified but without subsequent digits, then\n"
    L"        the number of digits will depend on the units. Enough digits will be\n"
    L"        printed to maintain full resolution of the unit to within one second.\n"
    L"        Thus, years: 8 digits, days: 5, hours: 4, minutes: 2.\n"
    L"\n"
    L"    Examples\n"
    L"         Given a delta time of 547,991,463 seconds, the following delta format\n"
    L"         strings will yield the following output:\n"
    L"\n"
    L"            %_S\n"
    L"                \"547991463\"\n"
    L"\n"
    L"            %_',.S\n"
    L"                \"547,991,463\"\n"
    L"\n"
    L"            %_Y years, %_yD days, %_dH. hours\n"
    L"                \"17 years, 137 days, 11.8508 hours\"\n"
    L"\n"
    L"    See `--time examples` for more example uses of delta time formats.\n"
    ;

static auto help_timeSyntax =
    L"\n"
    L"    The explicit '--time' option supports a variety of different formats,\n"
    L"    using the ISO 8601 date/time format.\n"
    L"\n"
    L"    <To be completed>\n"
    ;

static auto help_timeZone =
    L"\n"
    L"    Time zones have the format \"tzn[+|-]hh[:mm[:ss]][dzn]\", where\n"
    L"\n"
    L"        tzn\n"
    L"            Three-letter time-zone name, such as PST. You must specify the\n"
    L"            correct offset from local time to UTC.\n"
    L"\n"
    L"        hh\n"
    L"            Difference in hours between UTC and local time. Optionally signed.\n"
    L"\n"
    L"        mm\n"
    L"            Minutes, separated with a colon (:).\n"
    L"\n"
    L"        ss\n"
    L"            Seconds, separated with a colon (:).\n"
    L"\n"
    L"        dzn\n"
    L"            Three-letter daylight-saving-time zone such as PDT. If daylight\n"
    L"            saving time is never in effect in the locality, omit dzn. The C\n"
    L"            run-time library assumes the US rules for implementing the\n"
    L"            calculation of Daylight Saving Time (DST).\n"
    L"\n"
    L"        Examples of the timezone string include the following:\n"
    L"\n"
    L"            UTC       Universal Coordinated Time\n"
    L"            PST8      Pacific Standard Time\n"
    L"            PST8PDT   Pacific Standard Time, daylight savings in effect\n"
    L"            GST-1GDT  German Standard Time, daylight savings in effect\n"
    ;

static auto help_examples =
    L"\n"
    L"    Examples:\n"
    L"\n"
    L"    > timeprint\n"
    L"    Sunday, July 20, 2003 17:02:39\n"
    L"\n"
    L"    > timeprint %H:%M:%S\n"
    L"    17:03:17\n"
    L"\n"
    L"    > timeprint -z UTC\n"
    L"    Monday, July 21, 2003 00:03:47\n"
    L"\n"
    L"    > timeprint Building endzones [%Y-%m-%d %#I:%M:%S %p].\n"
    L"    Building endzones [2003-07-20 5:06:09 PM].\n"
    L"\n"
    L"    > echo. >timestamp.txt\n"
    L"    [a day and a half later...]\n"
    L"    > timeprint --modification timestamp.txt --now Elapsed Time: %_S seconds\n"
    L"    Elapsed Time: 129797 seconds\n"
    L"    > timeprint --modification timestamp.txt --now Elapsed Time: %_H:%_hM:%_mS\n"
    L"    Elapsed Time: 36:3:17\n"
    ;

//__________________________________________________________________________________________________
void help (HelpType type)
{
    // For HelpType::None, do nothing. For other help types, print corresponding help information
    // and exit.

    switch (type) {
        default: return;

        case HelpType::General:      _putws(help_general);      break;
        case HelpType::Examples:     _putws(help_examples);     break;
        case HelpType::DeltaTime:    _putws(help_deltaTime);    break;
        case HelpType::FormatCodes:  _putws(help_formatCodes);  break;
        case HelpType::TimeSyntax:   _putws(help_timeSyntax);   break;
        case HelpType::TimeZone:     _putws(help_timeZone);     break;
    }

    exit (0);
}
