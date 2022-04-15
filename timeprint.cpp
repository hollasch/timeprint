/*******************************************************************************
This program prints the current date and time to the standard output stream.
It takes an optional format string to control the output.
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>

#include <cstdarg>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

using std::time_t;
using std::tm;
using std::vector;
using std::wcout;
using std::wstring;

static auto version = L"timeprint 3.0.0-alpha.19 | 2022-04-07 | https://github.com/hollasch/timeprint";

enum class HelpType {
    // Types of usage information for the --help option

    None,
    Version,
    General,
    Examples,
    DeltaTime,
    FormatCodes,
    TimeSyntax,
    TimeZone,
};

enum class OptionType {
    // Command-Line Option Types

    None,
    AccessTime,
    CodeChar,
    CreationTime,
    Help,
    ModificationTime,
    Now,
    Time,
    TimeZone,
    Version,
};

enum class TimeType {
    // Type of time for an associated time value string

    None,         // Not a legal time
    Now,          // Current time
    Explicit,     // Explicit ISO-8601 date/time
    Access,       // Access time of the named file
    Creation,     // Creation time of the named file
    Modification  // Modification time of the named file
};

class TimeSpec {
  public:
    TimeType type { TimeType::None };   // Type of time
    wstring  value;                     // String value of specified type

    void Set(TimeType t, const wstring& str) {
        type = t;
        value = str;
    }

    void Set(TimeType t) {
        type = t;
        value.clear();
    }
};

class Parameters {
    // Describes the parameters for a run of this program.

  public:
    wchar_t  codeChar { L'%' };            // Format Code Character (default '%')
    HelpType helpType { HelpType::None };  // Type of help information to print & exit
    wstring  zone;                         // Time zone string
    wstring  format;                       // Output format string
    bool     isDelta  { false };           // Time calculation is a difference between two times

    TimeSpec time1;         // Time 1 [required] (either single use, or for time difference)
    TimeSpec time2;         // Time 2 [optional] (for time difference output)
};


// Global Constants
static const int secondsPerMinute       = 60;
static const int secondsPerHour         = secondsPerMinute * 60;
static const int secondsPerDay          = secondsPerHour * 24;
static const int secondsPerNominalYear  = secondsPerDay * 365;
static const int secondsPerTropicalYear = secondsPerNominalYear + (secondsPerDay / 400) * 497;   // 365+97/400 days


// Global Variables
static time_t currentTime;
static tm     currentTimeLocal;
static tm     currentTimeUTC;
static int    timeZoneOffsetHours;      // Signed hours offset from UTC
static int    timeZoneOffsetMinutes;    // Signed minutes offset from UTC


// Function Declarations
bool       calcTime           (const Parameters&, tm& timeValue, time_t& deltaTimeSeconds);
bool       charIn             (wchar_t c, const wchar_t* list);
wstring    defaultTimeFormat  (bool deltaFormat);
bool       equalIgnoreCase    (const wchar_t*, const wchar_t*);
bool       errorMsg           (const wchar_t *message, ...);
void       getCurrentTime     ();
bool       getDeltaNumberFormat (wstring::iterator& formatIterator, const wstring::iterator& formatEnd,
                                 wchar_t& thousandsChar, wchar_t& decimalChar);
OptionType getOptionType      (int& argi, int& paramOffset, wchar_t* argv[]);
bool       getParameters      (Parameters&, int argc, wchar_t* argv[]);
bool       getTimeFromSpec    (time_t& result, const TimeSpec&);
bool       getExplicitDateTime(time_t& result, wstring timeSpec);
bool       getExplicitTime    (tm& result, wstring::iterator specBegin, wstring::iterator specEnd);
bool       getExplicitDate    (tm& result, wstring::iterator specBegin, wstring::iterator specEnd);
int        getNumIntDigits    (double);
void       help               (HelpType);
void       printResults       (wstring format, wchar_t codeChar, const tm& timeValue, time_t deltaTimeSeconds);
void       printDelta         (wstring::iterator& formatIterator, wchar_t codeChar, const wstring::iterator& formatEnd,
                               time_t deltaSec);
bool       printDeltaFunc     (wstring::iterator& formatIterator, const wstring::iterator& formatEnd, time_t deltaSec);


//__________________________________________________________________________________________________
int wmain (int argc, wchar_t *argv[]) {
    Parameters params;

    if (!getParameters(params, argc, argv)) return -1;

    help (params.helpType);

    tm     calculatedTime;
    time_t deltaTimeSeconds;

    if (calcTime (params, calculatedTime, deltaTimeSeconds)) {
        printResults (params.format, params.codeChar, calculatedTime, deltaTimeSeconds);
        return 0;
    }

    return 1;
}


//__________________________________________________________________________________________________
void getCurrentTime () {
    // This function gets the current local time, and the corresponding local and UTC time structs.
    // It also gets the current time zone's hour & minute offsets from UTC.

    currentTime = std::time(nullptr);
    localtime_s (&currentTimeLocal, &currentTime);
    gmtime_s (&currentTimeUTC, &currentTime);

    // Figure out the local time zone's offset. Computing the difference between local and UTC times
    // is problematic, particularly when the local time zone is changing in or out of daylight
    // saving time. Given this, the best approach is to get the actual time zone offset from the
    // system ftime() function family. This returns a string of the form "[+|-]HHMM".

    wchar_t buffer[8];

    wcsftime (buffer, std::size(buffer), L"%z", &currentTimeLocal);
    timeZoneOffsetHours   = 10*(buffer[1]-L'0') + (buffer[2]-L'0');  // Parse out unsigned offset hours
    timeZoneOffsetMinutes = 10*(buffer[3]-L'0') + (buffer[4]-L'0');  // Parse out unsigned offset minutes

    // Handle possible negative sign.
    if (buffer[0] == L'-') {
        timeZoneOffsetHours   = -timeZoneOffsetHours;
        timeZoneOffsetMinutes = -timeZoneOffsetMinutes;
    }
}


//__________________________________________________________________________________________________
bool getParameters (Parameters &params, int argc, wchar_t* argv[]) {
    // This function processes the command line arguments and sets the corresponding values in the
    // Parameters structure. This function returns true if all arguments were legal and processed
    // properly, otherwise it returns false.

    // Process command arguments.
    auto argi = 1;
    while (argi < argc) {

        int paramOffset;
        auto arg = argv[argi];
        auto priorArgi = argi;         // Used later for reporting on invalid third time values.

        auto optionType = getOptionType(argi, paramOffset, argv);

        // Non-option arguments accrue to the output format string.
        if (optionType == OptionType::None) {
            if (!params.format.empty())
                params.format += L" ";
            params.format += arg;
            continue;
        }

        TimeSpec newTimeSpec;

        if (optionType == OptionType::Now) {
            newTimeSpec.Set(TimeType::Now);
        } else if (optionType == OptionType::Version) {
            params.helpType = HelpType::Version;
            return true;
        } else {

            wchar_t* parameter = (argi >= argc) ? nullptr : (argv[argi] + paramOffset);

            if (optionType == OptionType::Help) {
                // The help option may or may not take a parameter.
                params.helpType = (parameter == nullptr)                     ? HelpType::General
                                : equalIgnoreCase(parameter, L"examples")    ? HelpType::Examples
                                : equalIgnoreCase(parameter, L"deltaTime")   ? HelpType::DeltaTime
                                : equalIgnoreCase(parameter, L"deltaTimes")  ? HelpType::DeltaTime
                                : equalIgnoreCase(parameter, L"formatCode")  ? HelpType::FormatCodes
                                : equalIgnoreCase(parameter, L"formatCodes") ? HelpType::FormatCodes
                                : equalIgnoreCase(parameter, L"timeSyntax")  ? HelpType::TimeSyntax
                                : equalIgnoreCase(parameter, L"timeZone")    ? HelpType::TimeZone
                                : equalIgnoreCase(parameter, L"timeZones")   ? HelpType::TimeZone
                                : HelpType::General;
                return true;
            } else {
                // Options that always take parameters.

                if (optionType == OptionType::AccessTime) {
                    if (!parameter)
                        return errorMsg(L"Missing argument for %s option", arg);
                    newTimeSpec.Set(TimeType::Access, parameter);

                } else if (optionType == OptionType::CodeChar) {
                    if (!parameter)
                        return errorMsg(L"Missing argument for %s option", arg);
                    params.codeChar = *parameter;

                } else if (optionType == OptionType::CreationTime) {
                    if (!parameter)
                        return errorMsg(L"Missing argument for %s option", arg);
                    newTimeSpec.Set(TimeType::Creation, parameter);

                } else if (optionType == OptionType::ModificationTime) {
                    if (!parameter)
                        return errorMsg(L"Missing argument for %s option", arg);
                    newTimeSpec.Set(TimeType::Modification, parameter);

                } else if (optionType == OptionType::Time) {
                    if (!parameter)
                        return errorMsg(L"Missing argument for %s option", arg);
                    newTimeSpec.Set(TimeType::Explicit, parameter);

                } else if (optionType == OptionType::TimeZone) {
                    if (!parameter)
                        return errorMsg(L"Missing argument for %s option", arg);
                    params.zone = parameter;
                }

                ++argi;
            }
        }

        // Add in new (first or second) time spec.
        if (newTimeSpec.type != TimeType::None) {
            if (params.time1.type == TimeType::None) {
                params.time1 = newTimeSpec;
            } else if (params.time2.type == TimeType::None) {
                params.time2 = newTimeSpec;
                params.isDelta = true;
            } else {
                auto twoArgs = optionType != OptionType::Now;
                return errorMsg (L"Unexpected third time value (%s%s%s)",
                    argv[priorArgi], twoArgs ? L" " : L"", twoArgs ? argv[priorArgi+1] : L"");
            }
        }
    }

    // If no time source was specified, then report information for the current time.
    if (params.time1.type == TimeType::None)
        params.time1.Set(TimeType::Now);

    // If no format string was specified on the command line, use the default time format.
    if (params.format.empty()) {
        params.format = defaultTimeFormat (params.isDelta);
        params.codeChar = L'%';
    }

    return true;
}


//__________________________________________________________________________________________________
wstring defaultTimeFormat (bool deltaFormat) {
    // Returns the default time format for the absolute or delta time, either from the user's
    // environment variable, or from a standard default time format.

    wchar_t* timeFormatEnv;
    wstring  defaultFormat;

    if (deltaFormat) {
        _wdupenv_s (&timeFormatEnv, nullptr, L"TimeFormat_Delta");
        defaultFormat = timeFormatEnv ? timeFormatEnv : L"%_Y years, %_yD days, %_d0H:%_h0M:%_m0S";
    } else {
        _wdupenv_s (&timeFormatEnv, nullptr, L"TimeFormat");
        defaultFormat = timeFormatEnv ? timeFormatEnv : L"%#c";
    }

    free (timeFormatEnv);

    return defaultFormat;
}


//__________________________________________________________________________________________________
OptionType getOptionType (int& argi, int& paramOffset, wchar_t* argv[]) {
    // This function returns the type of the current option (indexed by `argi`). The option
    // parameter may immediately follow a single-letter option (for example, `-t12:00`). In this
    // case, `paramOffset` will be 2 and `argi` will not change, otherwise `argi` will be
    // incremented and `paramOffset` will be 0. Finally, the option type will be returned.
    // Unrecognized options will return OptionType::None, `argi` will be left unchanged, and
    // paramOffset will be 0.

    auto arg = argv[argi];
    paramOffset = 0;
    auto optionType = OptionType::None;

    // Check double-dash options and other options that take no arguments.

    struct {
        const wchar_t* optString;
        OptionType     type;
    } optionStrings[] = {
        { L"/?",             OptionType::Help },
        { L"-n",             OptionType::Now },
        { L"--accessed",     OptionType::AccessTime },
        { L"--access",       OptionType::AccessTime },
        { L"--codeChar",     OptionType::CodeChar },
        { L"--created",      OptionType::CreationTime },
        { L"--create",       OptionType::CreationTime },
        { L"--creation",     OptionType::CreationTime },
        { L"--help",         OptionType::Help },
        { L"--modified",     OptionType::ModificationTime },
        { L"--modify",       OptionType::ModificationTime },
        { L"--modification", OptionType::ModificationTime },
        { L"--now",          OptionType::Now },
        { L"--time",         OptionType::Time },
        { L"--timeZone",     OptionType::TimeZone },
        { L"--version",      OptionType::Version },
    };

    for (auto option : optionStrings) {
        if (equalIgnoreCase(arg, option.optString)) {
            ++argi;
            return option.type;
        }
    }

    // Single-dash options that may have (immediately-trailing) arguments.
    if (arg[0] == L'-') switch (tolower(arg[1])) {
        case L'%': optionType = OptionType::CodeChar;         break;
        case L'a': optionType = OptionType::AccessTime;       break;
        case L'c': optionType = OptionType::CreationTime;     break;
        case L'h': optionType = OptionType::Help;             break;
        case L'm': optionType = OptionType::ModificationTime; break;
        case L't': optionType = OptionType::Time;             break;
        case L'z': optionType = OptionType::TimeZone;         break;
    }

    // If we have a valid single-dash option and the parameter immediately follows, don't advance
    // the argument index, and set the parameter offset. Otherwise, advance to the next argument.
    if (optionType != OptionType::None && arg[2] != 0)
        paramOffset = 2;
    else
        ++argi;

    return optionType;
}


//__________________________________________________________________________________________________
bool calcTime (
    const Parameters& params,            // Command parameters
    tm&               timeValue,         // Output time value
    time_t&           deltaTimeSeconds)  // Output time delta in seconds
{
    // This function computes the time results and then sets the timeValue and deltaTimeSeconds
    // parameters. This function returns true on success, false on failure.

    // If an alternate time zone was specified, then we need to set the TZ environment variable.
    if (!params.zone.empty()) {
        _wputenv_s (L"TZ", params.zone.c_str());
    }

    getCurrentTime();    // Snapshot current time data to global variables.

    time_t time1;
    if (!getTimeFromSpec (time1, params.time1)) return false;

    if (params.time2.type == TimeType::None) {      // Reporting a single absolute time.
        deltaTimeSeconds = 0;
        localtime_s (&timeValue, &time1);
    } else {                                        // Reporting a time diffence
        time_t time2;
        if (!getTimeFromSpec (time2, params.time2)) return false;
        deltaTimeSeconds = (time1 < time2) ? (time2 - time1) : (time1 - time2);
        gmtime_s (&timeValue, &deltaTimeSeconds);
    }

    return true;
}


//__________________________________________________________________________________________________
bool getTimeFromSpec (time_t& result, const TimeSpec& spec) {
    // Gets the time according to the given time spec.

    if (spec.type == TimeType::Now) {
        result = currentTime;
        return true;
    }

    if (  (spec.type == TimeType::Access)
       || (spec.type == TimeType::Creation)
       || (spec.type == TimeType::Modification)) {

        struct _stat stat;    // File Status Data

        auto fileName = spec.value.c_str();

        if (0 != _wstat(fileName, &stat))
            return errorMsg(L"Couldn't get status of \"%s\"", fileName);

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

        return errorMsg(L"Unrecognized explicit time: \"%s\"", timeString.c_str());
    }

    return false;   // Unrecognized time type
}


//__________________________________________________________________________________________________
bool getExplicitDateTime (time_t& result, wstring timeSpec) {
    tm timeStruct = currentTimeLocal;

    auto dateTimeSep = std::find (timeSpec.begin(), timeSpec.end(), 'T');
    bool successResult;

    if (dateTimeSep != timeSpec.end()) {
        successResult = getExplicitTime (timeStruct, std::next(dateTimeSep), timeSpec.end())
                     && getExplicitDate (timeStruct, timeSpec.begin(), dateTimeSep);
    } else {
        successResult = getExplicitTime (timeStruct, timeSpec.begin(), timeSpec.end())
                     || getExplicitDate (timeStruct, timeSpec.begin(), timeSpec.end());
    }

    if (successResult) {
        if (timeStruct.tm_year < 70)
            return errorMsg(L"Cannot handle dates before 1970");
        timeStruct.tm_isdst = -1;         // DST status unknown
        result = mktime (&timeStruct);
    }

    return successResult;
}


//__________________________________________________________________________________________________
bool parseDateTimePatternCore (
    wchar_t*           pattern,
    wstring::iterator& sourceIt,
    wstring::iterator  sourceEnd,
    vector<int>&       results)
{
    // This is the core functionality of the explicit date & time parsing. Returns true if the given
    // pattern matches the source, and places the parsed integer results in the results vector.
    // Patterns may include the following characters:
    //
    //     #...    A sequence of digits yielding one number
    //     +       A sign character, either '+' or '-'. Yields +1 or -1, respectively.
    //     -       An optional dash.
    //     =       A mandatory dash.
    //     :       An optional colon.
    //     ...     Anything else must match exactly.

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


//__________________________________________________________________________________________________
bool parseDateTimePattern (
    wchar_t*           pattern,
    wstring::iterator& sourceIt,
    wstring::iterator  sourceEnd,
    vector<int>&       results)
{
    // Parses a date/time pattern. On failure, restores the sourceIt iterator and returns false,
    // otherwise on success leaves the sourceIt where it ended and returns true.

    wstring::iterator sourceReset = sourceIt;

    if (!parseDateTimePatternCore (pattern, sourceIt, sourceEnd, results)) {
        sourceIt = sourceReset;
        return false;
    }

    return true;
}


//__________________________________________________________________________________________________
bool getExplicitTime (tm& resultTimeLocal, wstring::iterator specBegin, wstring::iterator specEnd) {
    bool gotTime = false;
    vector<int> results;
    wstring::iterator specIt = specBegin;

    if (parseDateTimePattern (L"##:##:##", specIt, specEnd, results)) {
        resultTimeLocal.tm_hour = results[0];
        resultTimeLocal.tm_min  = results[1];
        resultTimeLocal.tm_sec  = results[2];
        gotTime = true;
    } else if (parseDateTimePattern (L"##:##", specIt, specEnd, results)) {
        resultTimeLocal.tm_hour = results[0];
        resultTimeLocal.tm_min  = results[1];
        gotTime = true;
    } else if (parseDateTimePattern (L"##", specIt, specEnd, results)) {
        resultTimeLocal.tm_hour = results[0];
        gotTime = true;
    }

    if (!gotTime) return false;

    // Parse timezone, if any.

    if (specIt == specEnd) {
        // Time was specified in local time, no conversion needed.
        return true;
    }

    if ((specIt[0] == L'Z') && (std::next(specIt) == specEnd)) {
        // UTC time; convert to local. We just do this manually by applying the offset.
        resultTimeLocal.tm_hour += timeZoneOffsetHours;
        resultTimeLocal.tm_min  += timeZoneOffsetMinutes;
        return true;
    }

    // Attempt to parse the explicit timezone from the spec.
    auto specOffsetHours   = 0;
    auto specOffsetMinutes = 0;

    if (parseDateTimePattern (L"+##:##", specIt, specEnd, results)) {
        specOffsetHours   = results[0] * results[1];
        specOffsetMinutes = results[0] * results[2];
    } else if (parseDateTimePattern (L"+##", specIt, specEnd, results)) {
        specOffsetHours = results[0] * results[1];
    }

    if (specIt != specEnd) return false;

    // Convert from specified time zone to UTC, then to local time.
    resultTimeLocal.tm_hour += -specOffsetHours   + timeZoneOffsetHours;
    resultTimeLocal.tm_min  += -specOffsetMinutes + timeZoneOffsetMinutes;

    return true;
}


//__________________________________________________________________________________________________
bool getExplicitDate (tm& resultTime, wstring::iterator specBegin, wstring::iterator specEnd) {
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
    wstring   format,             // The format string, possibly with escape sequences and format codes
    wchar_t   codeChar,           // The format code character (normally %)
    const tm& timeValue,          // The primary time value to use
    time_t    deltaTimeSeconds)   // Time difference when comparing two times
{
    // This procedure scans through the format string, emitting expanded codes and escape sequences
    // along the way.

    auto formatEnd = format.end();

    for (auto formatIterator = format.begin();  formatIterator != formatEnd;  ++formatIterator) {
        const auto buffSize = 1024;
        wchar_t    outputBuffer [buffSize];     // Intermediate Output Buffer

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

            const static auto legalCodes = L"aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ";

            wchar_t token[4];    // Code Token Word

            ++formatIterator;

            if (*formatIterator == L'_') {
                printDelta (++formatIterator, codeChar, formatEnd, deltaTimeSeconds);
                --formatIterator;  // Reset iterator for loop increment.

            } else if (*formatIterator == L'-' || isdigit(*formatIterator)) {
                // Numeric prefixed code.
                auto saveMark = formatIterator;     // Mark start in case of parse error.

                auto numPrefix = 0;
                auto numSign = 1;
                if (*formatIterator == L'-') {
                    ++formatIterator;
                    numSign = -1;
                }

                // Get the leading integer value before the code.
                wchar_t c;
                while (c = *formatIterator++, c && isdigit(c))
                    numPrefix = (10 * numPrefix) + (c - L'0');

                numPrefix *= numSign;

                const static auto legalPrefixedCodes = L"a";
                if (!c || !wcschr(legalPrefixedCodes, c) || numPrefix < 1) {
                    // If the string ended without a code character, or it's not a code that can
                    // take a numeric prefix, or the prefix is out of range, then reset and just
                    // emit without interpretation.
                    formatIterator = saveMark;
                    putwchar(codeChar);
                    putwchar(*formatIterator);
                } else {
                    // Only %a can take a numeric prefix for now.
                    wcsftime (outputBuffer, std::size(outputBuffer), L"%A", &timeValue);
                    if (numPrefix < wcslen(outputBuffer))
                        outputBuffer[numPrefix] = 0;
                    fputws (outputBuffer, stdout);
                    --formatIterator;
                }

            } else if (*formatIterator == codeChar) {
                putwchar (codeChar);
            } else if ((*formatIterator != L'#') && !wcschr(legalCodes, *formatIterator)) {
                // Print out illegal codes as-is.
                putwchar (codeChar);
                putwchar (*formatIterator);

            } else if ((formatIterator[0] == L'#') && !wcschr(legalCodes, formatIterator[1])) {
                // Print out illegal '#'-prefixed codes as-is.
                ++formatIterator;
                putwchar (codeChar);
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

                wcsftime (outputBuffer, std::size(outputBuffer), token, &timeValue);
                fputws (outputBuffer, stdout);
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


//__________________________________________________________________________________________________
void printDelta (
    wstring::iterator&       formatIterator,     // Pointer to delta format after '%_'
    wchar_t                  codeChar,           // Format Code Character
    const wstring::iterator& formatEnd,          // Format string end
    time_t                   deltaTimeSeconds)   // Time difference when comparing two times
{
    // This function attempts to print the time delta format. If the format is bad, then print the
    // format substring as-is and restore the format string pointer to continue.

    auto formatRestart = formatIterator;

    if (!printDeltaFunc(formatIterator, formatEnd, deltaTimeSeconds)) {
        putwchar (codeChar);
        putwchar (L'_');
        formatIterator = formatRestart;
    }
}


//__________________________________________________________________________________________________
bool printDeltaFunc (
    wstring::iterator&       formatIterator,     // Pointer to delta format after '%_'
    const wstring::iterator& formatEnd,          // Format string end
    time_t                   deltaTimeSeconds)   // Time difference when comparing two times
{
    if (formatIterator == formatEnd) return false;

    wchar_t thousandsChar;  // Thousands-separator character, 0=none
    wchar_t decimalChar;    // Decimal character
    if (!getDeltaNumberFormat (formatIterator, formatEnd, thousandsChar, decimalChar))
        return false;

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

    // Delta value, scaled
    double deltaValue = moduloUnit ? fmod(deltaTimeSeconds,moduloValue) : deltaTimeSeconds;

    // Parse delta unit.

    if (formatIterator == formatEnd) return false;

    auto unitType = *formatIterator++;
    auto leadingZeros = (moduloUnit && (unitType == '0')) ? 1 : 0;

    if (leadingZeros) {
        if (formatIterator == formatEnd) return false;
        unitType = *formatIterator++;
    }

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
            if (leadingZeros)
                leadingZeros = getNumIntDigits (moduloValue/secondsPerDay);
            break;
        }

        case L'H': {
            if (!charIn(moduloUnit, L"tyd")) return false; // Filter out invalid modulo unit prefixes.
            deltaValue /= secondsPerHour;
            if (leadingZeros)
                leadingZeros = getNumIntDigits (moduloValue/secondsPerHour);
            break;
        }

        case L'M': {
            if (!charIn(moduloUnit, L"tydh")) return false; // Filter out invalid modulo unit prefixes.
            deltaValue /= secondsPerMinute;
            if (leadingZeros)
                leadingZeros = getNumIntDigits (moduloValue/secondsPerMinute);
            break;
        }

        case L'S': {
            if (!charIn(moduloUnit, L"tydhm")) return false; // Filter out invalid modulo unit prefixes.
            if (leadingZeros)
                leadingZeros = getNumIntDigits (moduloValue);
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
    output << std::fixed << std::setprecision(precision);
    if (leadingZeros)
        output << std::setfill(L'0') << std::setw(leadingZeros);
    output << deltaValue;
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
bool getDeltaNumberFormat (
    wstring::iterator& formatIterator,
    const wstring::iterator& formatEnd,
    wchar_t& thousandsChar,
    wchar_t& decimalChar)
{
    // This function parses the thousands separator and decimal point formatting sequence, if it
    // exists. On return, `thousandsChar` will contain the thousands character, or zero if no
    // thousands character is to be printed, and `decimalChar` will contain the decimal character to
    // use, or 0 to use a '.' decimal point.

    thousandsChar = 0;
    decimalChar = 0;

    if (*formatIterator == L'\'') {
        if (++formatIterator == formatEnd) return false;
        thousandsChar = *formatIterator;
        if (++formatIterator == formatEnd) return false;
        decimalChar = *formatIterator;
        if (++formatIterator == formatEnd) return false;

        if (thousandsChar == L'0')    // '0' indicates no thousands character.
            thousandsChar = 0;
    }

    return true;
}


//__________________________________________________________________________________________________
bool charIn (wchar_t c, const wchar_t* list) {
    // Return true if the given character is in the zero-terminated array of characters.
    // Also returns true if c == 0.
    auto i = 0;
    while (list[i] && (c != list[i]))
        ++i;
    return (c == list[i]);
}


//__________________________________________________________________________________________________
int getNumIntDigits (double x) {
    // Returns the number of digits in the given integer value.

    int n = static_cast<int>(x);
    int nDigits = 1;
    while (n /= 10)
        ++ nDigits;

    return nDigits;
}


//__________________________________________________________________________________________________
bool equalIgnoreCase (const wchar_t* str1, const wchar_t* str2) {
    return 0 == _wcsicmp(str1, str2);
}


//__________________________________________________________________________________________________
bool errorMsg (const wchar_t *message, ...) {
    // Prints printf-style error message to stderr output stream. This function always returns false
    // (for chaining).

    va_list(arguments);
    va_start(arguments, message);

    wstring fullMessage = L"timeprint: ";
    fullMessage += message;
    fullMessage += L".\n";

    vfwprintf_s (stderr, fullMessage.c_str(), arguments);

    va_end(arguments);
    return false; 
}


//__________________________________________________________________________________________________
static auto help_general = LR"(
timeprint: Print time and date information
usage    : timeprint [--codeChar <char>] [-%<char>]
                     [--help [topic]] [-h[topic]] [/?] [--version]
                     [--<access|accessed> <file>] [-a<file>]
                     [--<create|created|creation> <file>] [-c<file>]
                     [--<modify|modified|modification> <file>] [-m<file>]
                     [--timeZone <zone>] [-z<zone>]
                     [--now] [-n]
                     [--time <timeValue>] [-t<timeValue>]
                     [string] ... [string]

This command prints time information to the standard output stream. All string
fragments will be concatenated with a space, so it's often unnecessary to quote
the format string.

timeprint operates in either absolute or differential mode. If one time value
is specified, then values for that absolute time are reported. If two time
values are supplied, then timeprint reports the values for the positive
difference between those two values. If no time values are given, then --now
is implied.

Single-letter command options that take an argument may be specified with or
without token separation. (For example, both `-htimeSyntax` and `-h timeSyntax`
are valid.)

    --help [topic], -h[topic], /?
        Print help and usage information in general, or for the optional
        specified topic. Topics include 'examples', 'deltaTime', 'formatCodes',
        'timeSyntax', and 'timezone'.

    --version
        Print version information.

    --codeChar <char>, -%<char>
        The --codeChar switch specifies an alternate code character to the
        default '%' character. If the backslash (\) is specified as the code
        character, then normal backslash escapes will be disabled. The
        --codeChar switch is ignored unless the format string is specified on
        the command line.

    --timeZone <zone>, -z<zone>
        The --timeZone argument takes a timezone string of the form used by the
        TZ environment variable and displays the result in that time zone. If no
        timezone is specified, the value in the TZ environment variable is used.
        If the environment variable TZ is unset, the system local time is used.
        For a description of the time zone format, use `--help timeZone`.

    --now, -n
        Use the current time. This is useful when specifying one of two time
        values for delta time printing. For absolute time printing, `--now` is
        the default.

    --time <value>, -t<value>
        Specifies an explicit absolute time, using ISO 8601 syntax. For a
        description of supported syntax, use `--help timeSyntax`.

    --access|--accessed <fileName>, -a<fileName>
        Use the time of last access of the named file for a time value.

    --create|--created|--creation <fileName>, -c<fileName>
        Use the creation time of the named file.

    --modify|--modified|--modification <fileName>, -m<fileName>
        Use the modification time of the named file.

If no output string is supplied, the format specified in the environment
variable TIMEFORMAT is used. If this variable is not set, then the format
defaults to "%#c". The TIMEFORMAT string must use the "%" code character.

Similarly, the default difference time format may be specified with the
TIMEFORMAT_DELTA environment variable. If this variable is not set, then the
format defaults to "%_Y years, %_yD days, %_d0H:%_h0M:%_m0S". The
TIMEFORMAT_DELTA string must use the "%" code character.

Note that if your format string begins with - or /, you will need to prefix it
with a \ character so that it is not confused with a command switch.

Strings take both \-escaped characters and %-codes in the style of printf.
The escape codes include \n (newline), \t (tab), \b (backspace),
\r (carriage return), and \a (alert, or beep).

For a full description of supported time format codes, use
`--help formatCodes`.

For additional help, use `--help <topic>`, where <topic> is one of:
    - examples
    - deltaTime
    - formatCodes
    - timeSyntax
    - timeZone
)";

//----------------------------------------------------------------------------------------------------------------------

static auto help_examples = LR"(
Examples
---------

    > timeprint
    Sunday, July 20, 2003 17:02:39

    > timeprint %H:%M:%S
    17:03:17

    > timeprint -z UTC
    Monday, July 21, 2003 00:03:47

    > timeprint Starting build at %Y-%m-%d %#I:%M:%S %p.
    Starting build at 2003-07-20 5:06:09 PM.

    > echo. >timestamp.txt
    [a day and a half later...]
    > timeprint --modification timestamp.txt --now Elapsed Time: %_S seconds
    Elapsed Time: 129797 seconds
    > timeprint --modification timestamp.txt --now Elapsed Time: %_H:%_hM:%_mS
    Elapsed Time: 36:3:17
)";

//----------------------------------------------------------------------------------------------------------------------

static auto help_deltaTime = LR"(
Delta Time Formatting
----------------------

    Time differences are reported using the delta time formats. The delta time
    format has the following syntax:

                               %_['kd][u[0]]<U>[.[#]]
                                  -v-  -v--  v  --v-
            Numeric Format --------'    |    |    |
            Next Greater Unit ----------'    |    |
            Units ---------------------------'    |
            Decimal Precision --------------------'

    Numeric Format ['kd] (_optional_)
        The optional `'` character is followed by two characters, k and d.
        k represents the character to use for the thousand's separator, with
        the special case that `0` indicates that there is to be no thousands
        separator. The d character is the character to use for the decimal
        point, if one is present. So, for example, `'0.` specifies no
        thousands separator, and the American `.` decimal point. `'.,` would
        specify European formatting, with `.` for the thousands separator, and
        `,` as the decimal point.

    Next Greater Unit [u[0]] (_optional_)
        This single lowercase letter indicates any preceding units used in the
        delta time printing. For example, if the unit is hours, and the next
        greater unit is years, then the hours reported are the remainder
        (modulo) after the number of years. Supported next greater units
        include the following:

            y - Nominal years (see units below for definition)
            t - Tropical years (see units below for definition)
            d - Days
            h - Hours
            m - Minutes

        If the next greater unit is followed by a zero, then the result is
        zero-padded to the appropriate width for the range of possible values.

    Units <U> (_required_)
        The unit of time (single uppercase letter) to report for the time
        delta. This is the remainder after the (optional) next greater unit.
        The following units are supported:

            Y - Nominal years
            T - Tropical years
            D - Days
            H - Hours
            M - Minutes
            S - Whole seconds

        Nominal years are 365 days in length.

        Tropical (or solar) years are approximately equal to one trip around
        the sun. These are useful to approximate the effect of leap years when
        reporting multi-year durations. For this program, a tropical year is
        defined as 365 + 97/400 days.

        The following are the supported combinations of next greater unit and
        unit:

            Y
            T
            D yD tD
            H yH tH dH
            M yM tM dM hM
            S yS tS dS hS mS

    Decimal Precision [.[#]] (_optional_)
        With the exception of seconds, all units will have a fractional value
        for time differences. If the decimal precision format is omitted, the
        then rounded whole value is printed.

        If the decimal point and number is specified, then the fractional
        value will be printed with the number of requested digits.

        If a decimal point is specified but without subsequent digits, then
        the number of digits will depend on the units. Enough digits will be
        printed to maintain full resolution of the unit to within one second.
        Thus, years: 8 digits, days: 5, hours: 4, minutes: 2.

    Examples
         Given a delta time of 547,991,463 seconds, the following delta format
         strings will yield the following output:

            %_S
                '547991463'

            %_',.S
                '547,991,463'

            %_Y years, %_yD days, %_dH. hours
                '17 years, 137 days, 11.8508 hours'

    See `--time examples` for more example uses of delta time formats.
)";

//----------------------------------------------------------------------------------------------------------------------

static auto help_formatCodes = LR"(
Format Codes
-------------

    The following time format codes are supported:

        %a     Abbreviated weekday name *
        %<d>a  Weekday name, abbreviated to d characters (min 1)
        %A     Full weekday name *
        %b     Abbreviated month name *
        %B     Full month name *
        %c     Date and time representation *
        %C     Year divided by 100 and truncated to integer (00-99)
        %d     Day of month as decimal number (01-31)
        %D     Short MM/DD/YY date, equivalent to %m/%d/%y
        %e     Day of the month, space-padded ( 1-31)
        %F     Short YYYY-MM-DD date, equivalent to %Y-%m-%d
        %g     Week-based year, last two digits (00-99)
        %G     Week-based year
        %h     Abbreviated month name (same as %b) *
        %H     Hour in 24-hour format (00-23)
        %I     Hour in 12-hour format (01-12)
        %j     Day of year as decimal number (001-366)
        %m     Month as decimal number (01-12)
        %M     Minute as decimal number (00-59)
        %n     New line character (same as '\n')
        %p     AM or PM designation
        %r     12-hour clock time *
        %R     24-hour HH:MM time, equivalent to %H:%M
        %S     Seconds as a decimal number (00-59)
        %t     Horizontal tab character (same as '\t')
        %T     ISO 8601 time format (HH:MM:SS) equivalent to %H:%M:%S
        %u     ISO 8601 weekday as number with Monday=1 (1-7)
        %U     Week number, first Sunday = week 1 day 1 (00-53)
        %V     ISO 8601 week number (01-53)
        %w     Weekday as decimal number, Sunday = 0 (0-6)
        %W     Week of year, decimal, Monday = week 1 day 1(00-51)
        %x     Date representation *
        %X     Time representation *
        %y     Year without century, as decimal number (00-99)
        %Y     Year with century, as decimal number
        %z     ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100)
               If timezone cannot be determined, no characters
        %Z     Time-zone name or abbreviation, empty for unrecognized zones *
        %_...  Delta time formats. See `--help deltaTime`.
        %%     Percent sign

        * Specifiers marked with an asterisk are locale-dependent.

    As in the printf function, the # flag may prefix any formatting code. In
    that case, the meaning of the format code is changed as follows.

        %#c
            Long date and time representation, appropriate for current locale.
            For example: Tuesday, March 14, 1995, 12:41:29.

        %#x
            Long date representation, appropriate to current locale.
            For example: Tuesday, March 14, 1995.

        %#d, %#H, %#I, %#j, %#m, %#M, %#S, %#U, %#w, %#W, %#y, %#Y
            Remove any leading zeros.

        All others
            The flag is ignored.
)";

//----------------------------------------------------------------------------------------------------------------------

static auto help_timeSyntax = LR"(
Time Syntax
------------

    The explicit `--time` option supports a variety of different formats,
    based on the ISO 8601 date/time format.

    An explicit date-time may have a date, a time, or both. In the case of
    both, they must be separated by the letter `T`. No spaces are allowed in
    the string.

    The date can take one of the following patterns, where a `=` character
    denotes a required dash, and a `-` denotes an optional dash:

        YYYY-MM-DD
        YYYY=MM
        YYYY
        ==MM-DD
        YYYY-DDD   (DDD = day of the year)

    The time can take one of the following patterns, where the `:` characters
    are optional:

        HH:MM:SS
        HH:MM
        HH

    The time may be followed by an optional time zone, which has the following
    pattern, where `+` represents a required `+` or `-` character.

        +HHMM    (Offset from UTC)
        +HH      (Offset from UTC)
        Z        (Zulu, or UTC)

    Parsing the explicit time value takes place as follows: if the string
    contains a `T`, then the date is parsed before the `T`, and the time is
    parsed after. If the string contains no `T`, then time parsing is first
    attempted, and on failure date parsing is attempted. Again, parsing is
    strict, and no other characters may included anywhere.

    Any unspecified units get the current time value for that unit.

    Example explicit time values include the following:

        2018-02-24T20:58:46-0800
        2018-02-25T04:58:46Z
        17:57
        --05-07
        120000Z
        1997-183
        19731217T113618-0700

    See `--help examples` for other examples.
)";

//----------------------------------------------------------------------------------------------------------------------

static auto help_timeZone = LR"(
Time Zones
-----------

    The time zone value may be specified with the TZ environment variable,
    or using the `--timezone` option. Time zones have the format
    `tzn[+|-]hh[:mm[:ss]][dzn]`, where

        tzn
            Time-zone name, three letters or more, such as PST.

        [+|-]hh
            The time that must be ADDED to local time to get UTC.
            CAREFUL: Unfortunately, this value is negated from how time zones
            are normally specified. For example, PDT is specified as -0800,
            but in the time zone string, will be specified as `PDT+08`.
            You can experiment with the string "%#c %Z %z" and the
            `--timezone` option to ensure you understand how these work
            together. If offset hours are omitted, they are assumed to be
            zero.

        [:mm]
            Minutes, prefixed with mandatory colon.

        [:ss]
            Seconds, prefixed with mandatory colon.

        [dzn]
            Three-letter daylight-saving-time zone such as PDT. If daylight
            saving time is never in effect in the locality, omit dzn. The C
            run-time library assumes the US rules for implementing the
            calculation of Daylight Saving Time (DST).

        Examples of the timezone string include the following:

            UTC       Universal Coordinated Time
            PST8      Pacific Standard Time
            PDT+07    Pacific Daylight Time
            NST+03:30 Newfoundland Standard Time
            PST8PDT   Pacific Standard Time, daylight savings in effect
            GST-1GDT  German Standard Time, daylight savings in effect
)";


//__________________________________________________________________________________________________
void help (HelpType type) {
    // For HelpType::None, do nothing. For other help types, print corresponding help information
    // and exit.

    switch (type) {
        default: return;

        case HelpType::General:
            _putws(help_general);
            _putws(version);
            break;

        case HelpType::Version:      _putws(version);           break;
        case HelpType::Examples:     _putws(help_examples);     break;
        case HelpType::DeltaTime:    _putws(help_deltaTime);    break;
        case HelpType::FormatCodes:  _putws(help_formatCodes);  break;
        case HelpType::TimeSyntax:   _putws(help_timeSyntax);   break;
        case HelpType::TimeZone:     _putws(help_timeZone);     break;
    }

    exit (0);
}
