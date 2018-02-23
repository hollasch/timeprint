/*******************************************************************************
This program prints the current date and time to the standard output stream.
It takes an optional format string to control the output.
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>

#include <string>
#include <sstream>
#include <iomanip>

using std::string;


enum class HelpType    // Types of usage information for the --help option
{
    None,         // No help information requested
    General,      // General usage information
    Examples,     // Illustrative examples
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
    string   value;    // String value of specified type
};


struct Parameters
{
    // Describes the parameters for a run of this program.

    char     codeChar;      // Format Code Character (default '%')
    HelpType helpType;      // Type of help information to print & exit
    string   zone;          // Time zone string
    string   format;        // Output format string

    TimeSpec time1;         // Time 1 [required] (either single use, or for time difference)
    TimeSpec time2;         // Time 2 [optional] (for time difference output)
};


// Global Variables
static time_t timeNow;


// Function Declarations
bool calcTime       (const Parameters& params, struct tm& timeValue, time_t& deltaTimeSeconds);
bool getParameters  (Parameters& params, int argc, char* argv[]);
bool getTime        (time_t& result, const TimeSpec&);
void help           (HelpType);
void printResults   (string format, char codeChar, const struct tm& timeValue, time_t deltaTimeSeconds);
void printDelta     (string::iterator& formatIterator, const string::iterator& formatEnd, time_t deltaTimeSeconds);
bool printDeltaFunc (string::iterator& formatIterator, const string::iterator& formatEnd, time_t deltaTimeSeconds);


//__________________________________________________________________________________________________
int main (int argc, char *argv[])
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
bool getParameters (Parameters &params, int argc, char* argv[])
{
    // This function processes the command line arguments and sets the corresponding values in the
    // Parameters structure. This function returns true if all arguments were legal and processed
    // properly, otherwise it returns false.

    // Set default values.
    params.codeChar = '%';
    params.helpType   = HelpType::None;
    params.time1.type = TimeType::None;
    params.time2.type = TimeType::None;

    // Process command arguments.
    for (auto i=1;  i < argc;  ++i) {
        auto argptr = argv[i];

        if (!((argv[i][0] == '-') || (argv[i][0] == '/'))) {
            if (params.format.empty())
                params.format += argptr;
            else {
                params.format += " ";
                params.format += argptr;
            }
        } else {
            auto timeValWord1 = i;         // Used later for reporting on invalid third time values.
            auto timeValWord2 = false;

            auto optChar = argv[i][1];     // Option Character

            if (optChar == 0) {
                fputs ("timeprint: Null option switch.\n", stderr);
                return false;
            }

            // Point argptr to the contents of the switch.  This may be immediately following the
            // option character, or it may be the next token on the command line.

            auto  advanceArg = (argv[i][2] == 0);
            char* switchWord = nullptr;

            if (optChar == '-') {
                advanceArg = true;
                switchWord = argv[i] + 2;
                if (0 == _stricmp(switchWord, "access"))
                    optChar = 'a';
                else if (0 == _stricmp(switchWord, "codeChar"))
                    optChar = '%';
                else if (0 == _stricmp(switchWord, "creation"))
                    optChar = 'c';
                else if (0 == _stricmp(switchWord, "help"))
                    optChar = 'h';
                else if (0 == _stricmp(switchWord, "modification"))
                    optChar = 'm';
                else if (0 == _stricmp(switchWord, "now")) {
                    optChar = 'n';
                    advanceArg = false;
                }
                else if (0 == _stricmp(switchWord, "time"))
                    optChar = 't';
                else if (0 == _stricmp(switchWord, "timeZone"))
                    optChar = 'z';
                else {
                    fprintf (stderr, "timeprint: Unrecognized switch (--%s).\n", switchWord);
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
                    fprintf (stderr, "timeprint: Unrecognized option (-%c).\n", optChar);
                    return false;

                // File Access Time
                case 'a':
                    if (!argptr) {
                        fprintf (stderr, "timeprint: Missing argument for --access (-a) option.\n");
                        return false;
                    }
                    newTimeType = TimeType::Access;
                    break;

                // File Modification Time
                case 'c':
                    if (!argptr) {
                        fprintf (stderr, "timeprint: Missing argument for --creation (-c) option.\n");
                        return false;
                    }
                    newTimeType = TimeType::Creation;
                    break;

                // Alternate Code Character
                case '%':
                    if (argptr) params.codeChar = *argptr;
                    break;

                // Command Usage & Help
                case 'H':
                case 'h':
                case '?':
                    if (!argptr) {
                        params.helpType = HelpType::General;
                    } else if (0 == _stricmp(argptr, "examples")) {
                        params.helpType = HelpType::Examples;
                    } else if (0 == _stricmp(argptr, "formatCodes")) {
                        params.helpType = HelpType::FormatCodes;
                    } else if (0 == _stricmp(argptr, "timeSyntax")) {
                        params.helpType = HelpType::TimeSyntax;
                    } else if (0 == _stricmp(argptr, "timeZone")) {
                        params.helpType = HelpType::TimeZone;
                    } else {
                        params.helpType = HelpType::General;
                    }
                    return true;

                // File Modification Time
                case 'm':
                    if (!argptr) {
                        fprintf (stderr, "timeprint: Missing argument for --modification (-m) option.\n");
                        return false;
                    }
                    newTimeType = TimeType::Modification;
                    break;

                case 'n':
                    newTimeType = TimeType::Now;
                    break;

                case 't':
                    if (!argptr) {
                        fprintf (stderr, "timeprint: Missing argument for --time (-t) option.\n");
                        return false;
                    }
                    newTimeType = TimeType::Explicit;
                    break;

                // Timezone
                case 'z':
                    if (!argptr) {
                        fprintf (stderr, "timeprint: Missing argument for --timeZone (-z) option.\n");
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
                    fprintf (stderr,
                        "timeprint: Unexpected third time value (%s%s%s).\n",
                        argv[timeValWord1],
                        timeValWord2 ? " " : "",
                        timeValWord2 ? argv[timeValWord1 + 1] : "");
                    return false;
                }
            }
        }
    }

    // If no format string was specified on the command line, fetch it from the TIMEFORMAT
    // environment variable.  If not available there, then use the default format string.

    if (params.format.empty()) {
        char *timeFormat;
        _dupenv_s (&timeFormat, nullptr, "TIMEFORMAT");

        if (!timeFormat) {
            params.format = "%#c";
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
        string zoneSet { "TZ=" + params.zone };
        _putenv (zoneSet.c_str());
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

        if (0 != _stat(fileName, &stat)) {
            fprintf (stderr, "timeprint: Couldn't get status of \"%s\".\n", fileName);
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
        auto timeString = spec.value.c_str();
        fprintf (stderr, "timeprint: Unrecognized explicit time (%s).\n", timeString);
        return false;
    }

    return false;   // Unrecognized time type
}


//__________________________________________________________________________________________________
void printResults (
    string           format,             // The format string, possibly with escape sequences and format codes
    char             codeChar,           // The format code character (normally %)
    const struct tm& timeValue,          // The primary time value to use
    time_t           deltaTimeSeconds)   // Time difference when comparing two times
{
    // This procedure scans through the format string, emitting expanded codes and escape sequences
    // along the way.

    auto formatEnd = format.end();

    for (auto formatIterator = format.begin();  formatIterator != formatEnd;  ++formatIterator) {
        const auto buffsize = 1024;
        char       buff [buffsize];        // Intermediate Output Buffer

        // Handle backslash sequences, unless backslash is the alternate escape character.

        if ((*formatIterator == '\\') && (codeChar != '\\')) {
            ++formatIterator;

            switch (*formatIterator) {
                // Unrecognized \-sequences resolve to the escaped character.

                default:   putchar(*formatIterator);  break;

                // If the string ends with a \, then just emit the \.

                case 0:    putchar('\\');  break;

                // Recognized \-sequences are handled here.

                case 'n':  putchar('\n');  break;
                case 't':  putchar('\t');  break;
                case 'b':  putchar('\b');  break;
                case 'r':  putchar('\r');  break;
                case 'a':  putchar('\a');  break;
            }

        } else if (*formatIterator == codeChar) {

            const static auto legalCodes = "%aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ";

            char token[4];    // Code Token Word

            ++formatIterator;

            if (*formatIterator == '_') {
                printDelta (++formatIterator, formatEnd, deltaTimeSeconds);
                --formatIterator;  // Reset iterator for loop increment.
            } else if ((*formatIterator != '#') && !strchr(legalCodes, *formatIterator)) {
                // Print out illegal codes as-is.
                putchar ('%');
                putchar (*formatIterator);
            } else if ((formatIterator[0] == '#') && !strchr(legalCodes, formatIterator[1])) {
                // Print out illegal '#'-prefixed codes as-is.
                ++formatIterator;
                putchar ('%');
                putchar ('#');
                putchar (*formatIterator);
            } else {
                // Standard legal strftime() Code Sequences
                token[0] = '%';
                token[1] = *formatIterator;
                token[2] = 0;

                if (*formatIterator == '#') {
                    token[2] = *++formatIterator;
                    token[3] = 0;
                }

                strftime (buff, sizeof(buff), token, &timeValue);
                fputs (buff, stdout);
            }

        } else {

            // All unescaped character are emitted as-is.

            putchar (*formatIterator);
        }
    }

    // Print the final newline.

    putchar ('\n');
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
    string::iterator&       formatIterator,     // Pointer to delta format after '%_'
    const string::iterator& formatEnd,          // Format string end
    time_t                  deltaTimeSeconds)   // Time difference when comparing two times
{
    // This function attempts to print the time delta format. If the format is bad, then print the
    // format substring as-is and restore the format string pointer to continue.

    auto formatRestart = formatIterator;

    if (!printDeltaFunc(formatIterator, formatEnd, deltaTimeSeconds)) {
        fputs ("%_", stdout);
        formatIterator = formatRestart;
    }
}


bool charIn (char c, const char* list)
{
    // Return true if the given character is in the zero-terminated array of characters.
    // Also returns true if c == 0.
    auto i = 0;
    while (list[i] && (c != list[i]))
        ++i;
    return (c == list[i]);
}


bool printDeltaFunc (
    string::iterator&       formatIterator,     // Pointer to delta format after '%_'
    const string::iterator& formatEnd,          // Format string end
    time_t                  deltaTimeSeconds)   // Time difference when comparing two times
{
    double deltaValue;              // Delta value, scaled
    char   thousandsChar = 0;       // Thousands-separator character, 0=none
    char   decimalChar = 0;         // Decimal character

    if (formatIterator == formatEnd) return false;

    // Parse thousands separator and decimal point format flag.
    if (*formatIterator == '\'') {
        if (++formatIterator == formatEnd) return false;
        thousandsChar = *formatIterator;
        if (++formatIterator == formatEnd) return false;
        decimalChar = *formatIterator;
        if (++formatIterator == formatEnd) return false;

        if (thousandsChar == '0')
            thousandsChar = 0;
    }

    // Parse modulo unit, if one exists.
    char   moduloUnit  = *formatIterator++;
    double moduloValue = 0;

    switch (moduloUnit) {
        case 'y':  moduloValue = secondsPerNominalYear;   break;
        case 't':  moduloValue = secondsPerTropicalYear;  break;
        case 'd':  moduloValue = secondsPerDay;           break;
        case 'h':  moduloValue = secondsPerHour;          break;
        case 'm':  moduloValue = secondsPerMinute;        break;

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
        case 'Y': {
            if (moduloUnit != 0) return false; // There are no legal modulo unit prefixes for year.
            deltaValue /= secondsPerNominalYear;
            break;
        }

        case 'T': {
            if (moduloUnit != 0) return false; // There are no legal modulo unit prefixes for year.
            deltaValue /= secondsPerTropicalYear;
            break;
        }

        case 'D': {
            if (!charIn(moduloUnit, "ty")) return false; // Filter out invalid modulo unit prefixes.
            deltaValue /= secondsPerDay;
            break;
        }

        case 'H': {
            if (!charIn(moduloUnit, "tyd")) return false; // Filter out invalid modulo unit prefixes.
            deltaValue /= secondsPerHour;
            break;
        }

        case 'M': {
            if (!charIn(moduloUnit, "tydh")) return false; // Filter out invalid modulo unit prefixes.
            deltaValue /= secondsPerMinute;
            break;
        }

        case 'S': {
            if (!charIn(moduloUnit, "tydhm")) return false; // Filter out invalid modulo unit prefixes.
            break;
        }

        default: return false;
    }

    // Determine the precision of the output value.

    std::ostringstream output;            // Number value string
    auto               precision = 0;     // Output decimal precision

    if (unitType == 'S') {
        // Seconds have no fractional value.
    } else if ((formatIterator == formatEnd) || (*formatIterator != '.')) {
        deltaValue = floor(deltaValue);
    } else {
        ++formatIterator;
        if ((formatIterator == formatEnd ) || !isdigit(*formatIterator)) {
            switch (unitType) {
                case 'T':
                case 'Y': precision = 8; break;
                case 'D': precision = 5; break;
                case 'H': precision = 4; break;
                case 'M': precision = 2; break;
            }
        } else {
            while ((formatIterator != formatEnd) && isdigit(*formatIterator))
                precision = 10*precision + (*formatIterator++ - '0');
        }
    }

    // Get the string value of the deltaValue with the requested precision.
    output << std::fixed << std::setprecision(precision) << deltaValue;
    string outputString = output.str();

    auto decimalPointIndex = outputString.rfind('.');

    // Replace decimal point if requested.
    if (decimalChar && (decimalPointIndex != string::npos))
        outputString.replace(decimalPointIndex, 1, 1, decimalChar);

    // Insert thousands separator character if requested.
    if (thousandsChar) {
        auto kGroupIndex = 0;

        if (decimalPointIndex == string::npos)
            kGroupIndex = static_cast<int>(outputString.length() - 3);
        else {
            kGroupIndex = static_cast<int>(decimalPointIndex - 3);
        }
        
        while (kGroupIndex > 0) {
            outputString.insert (kGroupIndex, 1, thousandsChar);
            kGroupIndex -= 3;
        }
    }

    fputs (outputString.c_str(), stdout);
    return true;
}


/*==================================================================================================
Temporary Notes: Legal ISO-8601 Formats for timeprint

Syntax
    Done   Syntax                            Missing Values
    -----  --------------------------------  -------------------------------------------------------
    [ ]    <timeValue> = now                 use current date + time + zone
    [ ]    <timeValue> = <date>              time = current time
    [ ]    <timeValue> = <time>              date = current date
    [ ]    <timeValue> = <date>T<time>

    [ ]    <date> = <YYYY>-?<MM>-?<DD>
    [ ]    <date> = <YYYY>                   month = current month, day = current day
    [ ]    <date> = <YYYY>-<MM>              day = current day
    [ ]    <date> = --<MM>-?<DD>             year = current year
    [ ]    <date> = <YYYY>-?<DDD>

    [ ]    <time> = <HH>:?<MM>:?<SS><zone>
    [ ]    <time> = <HH>:?<MM><zone>         seconds = current seconds
    [ ]    <time> = <HH><zone>               minutes = current minutes, seconds = current seconds

    [ ]    <zone> = <null>                   use current local time zone
    [ ]    <zone> = Z
    [ ]    <zone> = [+-]<HH>:?<MM>
    [ ]    <zone> = [+-]<HH>

    [ ]    -?     = - | <null>
    [ ]    :?     = : | <null>
    [ ]    +-     = + | -
    [ ]    <YYYY> = 0000-9999
    [ ]    <MM>   = 01-12
    [ ]    <DD>   = 01-31
    [ ]    <DDD>  = 001-366
    [ ]    <HH>   = 00-24                    24 = midnight / 00 of next day
    [ ]    <MM>   = 00-59
    [ ]    <SS>   = 00-60                    Accomodates leap seconds

==================================================================================================*/


//__________________________________________________________________________________________________
static auto help_general =
    "timeprint v2.0.0-beta  |  https://github.com/hollasch/timeprint\n"
    "timeprint - Print time and date information\n"
    "\n"
    "usage: timeprint [--codeChar <char>] [-%<char>]\n"
    "                 [--help [topic]] [-h[topic]] [/?]\n"
    "                 [--access <fileName>] [-a<fileName>]\n"
    "                 [--creation <fileName>] [-c<fileName>]\n"
    "                 [--modification <fileName>] [-m<fileName>]\n"
    "                 [--timeZone <zone>] [-z<zone>]\n"
    "                 [--now] [-n]\n"
    "                 [--time <timeValue>] [-t<timeValue>]\n"
    "                 [string] ... [string]\n"
    "\n"
    "This command prints time information to the standard output stream. All string\n"
    "fragments will be concatenated with a space, so it's usually unnecessary to\n"
    "quote the format string.\n"
    "\n"
    "timeprint operates in either absolute or differential mode. If one time value\n"
    "is specified, then values for that absolute time are reported. If two time\n"
    "values are supplied, then timeprint reports the values for the positive\n"
    "difference between those two values. If no time values are given, then --now\n"
    "is implied.\n"
    "\n"
    "Command switches may be prefixed with a dash (-) or a slash (/).\n"
    "\n"
    "    --access <fileName>, -a<fileName>\n"
    "        Use the time of last access of the named file for a time value.\n"
    "\n"
    "    --codeChar <char>, -%<char>\n"
    "        The --codeChar switch specifies an alternate code character to the\n"
    "        default '%' character. If the backslash (\\) is specified as the code\n"
    "        character, then normal backslash escapes will be disabled. The\n"
    "        --codeChar switch is ignored unless the format string is specified on\n"
    "        the command line.\n"
    "\n"
    "    --creation <fileName>, -c <fileName>\n"
    "        Use the creation time of the named file.\n"
    "\n"
    "    --help [topic], -h[topic], -?[topic]\n"
    "        Print help and usage information in general, or for the specified\n"
    "        topic. Topics include 'examples', 'formatCodes', 'timeSyntax', and\n"
    "        'timezone'.\n"
    "\n"
    "    --modification <fileName>, -m<fileName>\n"
    "        Use the modification time of the named file.\n"
    "\n"
    "    --now, -n\n"
    "        Use the current time.\n"
    "\n"
    "    --time <value>, -t<value>\n"
    "        Specifies an explicit absolute time, using ISO 8601 syntax. For a\n"
    "        description of supported syntax, use '--help timeSyntax'.\n"
    "\n"
    "    --timeZone <zone>, -z<zone>\n"
    "        The --timeZone argument takes a timezone string of the form used by\n"
    "        the _tzset function. If no timezone is specified, the system local\n"
    "        time is used. The timezone can be set in the environment via the TZ\n"
    "        environment variable. For a description of the time zone format, use\n"
    "        '--help timeZone'.\n"
    "\n"
    "If no output string is supplied, the format specified in the environment\n"
    "variable TIMEFORMAT is used. If this variable is not set, then the format\n"
    "defaults to \"%#c\".\n"
    "\n"
    "Note that if your format string begins with - or /, you will need to prefix it\n"
    "with a \\ character so that it is not confused with a command switch.\n"
    "\n"
    "Strings take both \\-escaped characters and %-codes in the style of printf.\n"
    "The escape codes include \\n (newline), \\t (tab), \\b (backspace),\n"
    "\\r (carriage return), and \\a (alert, or beep).\n"
    "\n"
    "For a full description of supported time format codes, use\n"
    "'--help formatCodes'.\n"
    "\n"
    "For additional help, use '--help <topic>', where <topic> is one of:\n"
    "    - examples\n"
    "    - formatCodes\n"
    "    - timeSyntax\n"
    "    - timeZone\n"
    ;

static auto help_formatCodes =
    "\n"
    "    The following time format codes are supported:\n"
    "\n"
    "    %a     Abbreviated weekday name *\n"
    "    %A     Full weekday name *\n"
    "    %b     Abbreviated month name *\n"
    "    %B     Full month name *\n"
    "    %c     Date and time representation *\n"
    "    %C     Year divided by 100 and truncated to integer (00-99)\n"
    "    %d     Day of month as decimal number (01-31)\n"
    "    %D     Short MM/DD/YY date, equivalent to %m/%d/%y\n"
    "    %e     Day of the month, space-padded ( 1-31)\n"
    "    %F     Short YYYY-MM-DD date, equivalent to %Y-%m-%d\n"
    "    %g     Week-based year, last two digits (00-99)\n"
    "    %G     Week-based year\n"
    "    %h     Abbreviated month name (same as %b) *\n"
    "    %H     Hour in 24-hour format (00-23)\n"
    "    %I     Hour in 12-hour format (01-12)\n"
    "    %j     Day of year as decimal number (001-366)\n"
    "    %m     Month as decimal number (01-12)\n"
    "    %M     Minute as decimal number (00-59)\n"
    "    %n     New line character (same as '\\n')\n"
    "    %p     AM or PM designation\n"
    "    %r     12-hour clock time *\n"
    "    %R     24-hour HH:MM time, equivalent to %H:%M\n"
    "    %S     Seconds as a decimal number (00-59)\n"
    "    %t     Horizontal tab character (same as '\\t')\n"
    "    %T     ISO 8601 time format (HH:MM:SS) equivalent to %H:%M:%S\n"
    "    %u     ISO 8601 weekday as number with Monday=1 (1-7)\n"
    "    %U     Week number, first Sunday = week 1 day 1 (00-53)\n"
    "    %V     ISO 8601 week number (01-53)\n"
    "    %w     Weekday as decimal number, Sunday = 0 (0-6)\n"
    "    %W     Week of year, decimal, Monday = week 1 day 1(00-51)\n"
    "    %x     Date representation *\n"
    "    %X     Time representation *\n"
    "    %y     Year without century, as decimal number (00-99)\n"
    "    %Y     Year with century, as decimal number\n"
    "    %z     ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100)\n"
    "           If timezone cannot be determined, no characters\n"
    "    %Z     Time-zone name or abbreviation, empty for unrecognized zones *\n"
    "    %_S    Elapsed whole seconds\n"
    "    %%     Percent sign\n"
    "\n"
    "    * Specifiers marked with an asterisk are locale-dependent.\n"
    "\n"
    "As in the printf function, the # flag may prefix any formatting code. In that\n"
    "case, the meaning of the format code is changed as follows.\n"
    "\n"
    "    %#c\n"
    "        Long date and time representation, appropriate for current locale.\n"
    "        For example: Tuesday, March 14, 1995, 12:41:29.\n"
    "\n"
    "    %#x\n"
    "        Long date representation, appropriate to current locale.\n"
    "        For example: Tuesday, March 14, 1995.\n"
    "\n"
    "    %#d, %#H, %#I, %#j, %#m, %#M, %#S, %#U, %#w, %#W, %#y, %#Y\n"
    "        Remove any leading zeros.\n"
    "\n"
    "    All others\n"
    "        The flag is ignored.\n"
    ;

static auto help_timeSyntax =
    "\n"
    "    The explicit '--time' option supports a variety of different formats,\n"
    "    using the ISO 8601 date/time format.\n"
    "\n"
    "    <To be completed>\n"
    ;

static auto help_timeZone =
    "\n"
    "    Time zones have the format \"tzn[+|-]hh[:mm[:ss]][dzn]\", where\n"
    "\n"
    "        tzn\n"
    "            Three-letter time-zone name, such as PST. You must specify the\n"
    "            correct offset from local time to UTC.\n"
    "\n"
    "        hh\n"
    "            Difference in hours between UTC and local time. Optionally signed.\n"
    "\n"
    "        mm\n"
    "            Minutes, separated with a colon (:).\n"
    "\n"
    "        ss\n"
    "            Seconds, separated with a colon (:).\n"
    "\n"
    "        dzn\n"
    "            Three-letter daylight-saving-time zone such as PDT. If daylight\n"
    "            saving time is never in effect in the locality, omit dzn. The C\n"
    "            run-time library assumes the US rules for implementing the\n"
    "            calculation of Daylight Saving Time (DST).\n"
    "\n"
    "        Examples of the timezone string include the following:\n"
    "\n"
    "            UTC       Universal Coordinated Time\n"
    "            PST8      Pacific Standard Time\n"
    "            PST8PDT   Pacific Standard Time, daylight savings in effect\n"
    "            GST-1GDT  German Standard Time, daylight savings in effect\n"
    ;

static auto help_examples =
    "\n"
    "    Examples:\n"
    "\n"
    "    > timeprint\n"
    "    Sunday, July 20, 2003 17:02:39\n"
    "\n"
    "    > timeprint %H:%M:%S\n"
    "    17:03:17\n"
    "\n"
    "    > timeprint -z UTC\n"
    "    Monday, July 21, 2003 00:03:47\n"
    "\n"
    "    > timeprint Building endzones [%Y-%m-%d %#I:%M:%S %p].\n"
    "    Building endzones [2003-07-20 5:06:09 PM].\n"
    "\n"
    "    > echo. >timestamp.txt\n"
    "    [a day and a half later...]\n"
    "    > timeprint --modification timestamp.txt --now Elapsed Time: %_S seconds\n"
    "    Elapsed Time: 129600 seconds\n"
    ;

//__________________________________________________________________________________________________
void help (HelpType type)
{
    // For HelpType::None, do nothing. For other help types, print corresponding help information
    // and exit.

    switch (type) {
        default: return;

        case HelpType::General:      puts(help_general);      break;
        case HelpType::Examples:     puts(help_examples);     break;
        case HelpType::FormatCodes:  puts(help_formatCodes);  break;
        case HelpType::TimeSyntax:   puts(help_timeSyntax);   break;
        case HelpType::TimeZone:     puts(help_timeZone);     break;
    }

    exit (0);
}
