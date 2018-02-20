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
#include <iostream>

using std::string;


enum class HelpType    // Types of usage information for the --help option
{
    None,     // No help information requested
    General   // General usage information
};


enum class TimeType    // Type of time for an associated time value string
{
    None,         // Not a legal time
    Now,          // Current time
    Explicit,     // Explicit ISO-8601 date/time
    Access,       // Access time of the named file
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
bool calcTime      (Parameters& params, struct tm& timeValue, time_t& deltaTimeSeconds);
bool getParameters (Parameters& params, int argc, char* argv[]);
bool getTime       (time_t& result, TimeSpec&);
void help          (HelpType);
void printResults  (string format, char codeChar, struct tm& timeValue, time_t deltaTimeSeconds);


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
                switchWord = argv[i] + 2;
                if (0 == _stricmp(switchWord, "accessTime"))
                    optChar = 'a';
                else if (0 == _stricmp(switchWord, "codeChar"))
                    optChar = 'c';
                else if (0 == _stricmp(switchWord, "help"))
                    optChar = 'h';
                else if (0 == _stricmp(switchWord, "modTime"))
                    optChar = 'm';
                else if (0 == _stricmp(switchWord, "now"))
                    optChar = 'n';
                else if (0 == _stricmp(switchWord, "time"))
                    optChar = 't';
                else if (0 == _stricmp(switchWord, "timeZone"))
                    optChar = 'z';
                else {
                    fprintf (stderr, "timeprint: Unrecognized switch (--%s).\n", switchWord);
                    return false;
                }
                advanceArg = true;
            }

            if (advanceArg) {
                ++i;
                if (i >= argc) {
                    argptr = 0;
                } else {
                    argptr = argv[i];
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
                        fprintf (stderr, "timeprint: Missing argument for --accessTime (-a) option.\n");
                        return false;
                    }
                    newTimeType = TimeType::Access;
                    break;

                // Alternate Code Character
                case 'c':
                    if (argptr) params.codeChar = *argptr;
                    break;

                // Command Usage & Help
                case 'H':
                case 'h':
                case '?':
                    params.helpType = HelpType::General;
                    return true;

                // File Modification Time
                case 'm':
                    if (!argptr) {
                        fprintf (stderr, "timeprint: Missing argument for --modTime (-m) option.\n");
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
                    fprintf (stderr, "timeprint: Unexpected third time value (%s).\n", argptr);
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
    Parameters& params,            // Command parameters
    struct tm&  timeValue,         // Output time value
    time_t&     deltaTimeSeconds)  // Output time delta in seconds
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
bool getTime (time_t& result, TimeSpec& spec)
{
    // Gets the time according to the spec's type + value.

    if (spec.type == TimeType::Now) {
        result = timeNow;
        return true;
    }

    if (  (spec.type == TimeType::Access)
       || (spec.type == TimeType::Modification)) {

        struct _stat stat;    // File Status Data

        auto modFileName = spec.value.c_str();

        if (0 != _stat(modFileName, &stat)) {
            fprintf (stderr, "timeprint: Couldn't get status of \"%s\".\n", modFileName);
            return false;
        }

        switch (spec.type) {
            case TimeType::Access:        result = stat.st_atime; break;
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
    string     format,             // The format string, possibly with escape sequences and format codes
    char       codeChar,           // The format code character (normally %)
    struct tm& timeValue,          // The primary time value to use
    time_t     deltaTimeSeconds)   // Time difference when comparing two times
{
    // This procedure scans through the format string, emitting expanded codes and escape sequences
    // along the way.

    for (auto formatIterator = format.begin();  formatIterator != format.end();  ++formatIterator) {
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
                time_t divisor = 1;      // Delta seconds divisor
                bool   bogus = false;    // True on bogus %_ codes.

                ++formatIterator;
                switch (*formatIterator) {
                    case 'd': divisor = 60 * 60 * 24; break;   // Elapsed days
                    case 'h': divisor = 60 * 60;      break;   // Elapsed hours
                    case 's': divisor = 1;            break;   // Elapsed seconds
                    default:  bogus = true;           break;   // Unrecognized %_ code
                }

                if (bogus) {
                    printf ("%%_%c", *formatIterator);
                } else {
                    printf ("%I64d", deltaTimeSeconds / divisor);
                }

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
    "usage: timeprint [--codeChar <char>] [-c<char>]\n"
    "                 [--help] [-h] [/?]\n"
    "                 [--modTime <fileName>] [-m<fileName>]\n"
    "                 [--accessTime <fileName>] [-a<accessTime>]\n"
    "                 [--timeZone <zone>] [-z<zone>]\n"
    "                 [string] ... [string]\n"
    "\n"
    "This command prints time information to the standard output stream. All string\n"
    "fragments will be concatenated with a space, so it's usually unnecessary to\n"
    "quote the format string.\n"
    "\n"
    "Command switches may be prefixed with a dash (-) or a slash (/).\n"
    "\n"
    "--accessTime, -a\n"
    "    Use the time of last access of the named file for a time value.\n"
    "\n"
    "--codeChar, -c\n"
    "    The codeChar switch specifies an alternate code character to the default\n"
    "    '%' character (format codes are described below). If the backslash (\\)\n"
    "    is specified as the code character, then normal backslash escapes will\n"
    "    be disabled. The --codeChar switch is ignored unless the format string is\n"
    "    specified on the command line.\n"
    "\n"
    "--help, -h, /h, -?, /?\n"
    "    Print help and usage information.\n"
    "\n"
    "--modTime, -m\n"
    "    The modTime switch specifies the name of a file whose modification time is\n"
    "    used as the base time (instead of 1970-01-01 00:00:00). This is useful for\n"
    "    reporting time elapsed since a given file's modification.\n"
    "\n"
    "--timeZone, -z\n"
    "    The timeZone argument takes a timezone string of the form used by the\n"
    "    _tzset function. If no timezone is specified, the system local time is\n"
    "    used. The timezone also be set in the environment via the TZ environment\n"
    "    variable. The format of this string is \"tzn[+|-]hh[:mm[:ss]][dzn]\", where\n"
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
    "\n"
    "If no output string is supplied, the format specified in the environment\n"
    "variable TIMEFORMAT is used. If this variable is not set, then the format\n"
    "defaults to \"%#c\".\n"
    "\n"
    "Note that if your format string begins with - or /, you will need to prefix it\n"
    "with a \\ character so that it is not confused with a command switch.\n"
    "\n"
    "Strings take both \\-escaped characters and %-codes in the style of printf.\n"
    "The \\ escape codes include \\n (newline), \\t (tab), \\b (backspace),\n"
    "\\r (carriage return), and \\a (alert, or beep).\n"
    "\n"
    "The %-codes are\n"
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
    "    %_d    Elapsed whole days\n"
    "    %_h    Elapsed whole hours\n"
    "    %_s    Elapsed whole seconds\n"
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
    "\n"
    "\n"
    "Examples:\n"
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
    "\n"
    "    [about a day and a half later...]\n"
    "\n"
    "    > timeprint -m timestamp.txt Elapsed Time: %_dd, %H:%M:%S\n"
    "    Elapsed Time: 1d, 12:03:47\n"
    "    > timeprint -m timestamp.txt Elapsed Time: %_h:%M:%S\n"
    "    Elapsed Time: 36:03:47\n"
    "    > timeprint -m timestamp.txt Elapsed Time: %_s seconds\n"
    "    Elapsed Time: 129827 seconds\n"
    "\n"
    ;

//__________________________________________________________________________________________________
void help (HelpType type)
{
    // For HelpType::None, do nothing. For other help types, print corresponding help information
    // and exit.

    switch (type) {
        default: return;

        case HelpType::General:  puts(help_general); break;
    }

    exit (0);
}
