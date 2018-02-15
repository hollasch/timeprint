/*******************************************************************************
This program prints the current date and time to the standard output stream.
It takes an optional format string to control the output.
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>


auto usage =
    "currtime v1.0.0  |  https://github.com/hollasch/currtime\n"
    "currtime:  Print current time and date\n"
    "usage   :  currtime [-e<char>] [-m<file>] [-z<timezone>] [string] ... [string]\n"
    "\n"
    "    This command prints time information to the standard output stream.\n"
    "\n"
    "    The -e switch specifies an alternate escape character to the default %\n"
    "    character (escape codes are described below).  If the backslash (\\) is\n"
    "    specified as the escape character, then normal backslash escapes will be\n"
    "    disabled.  The -e switch is ignored unless the format string is specified\n"
    "    on the command line.\n"
    "\n"
    "    The -m switch specifies a file whose modification time is used as the\n"
    "    base time (instead of 1970-01-01 00:00:00).  This is useful for reporting\n"
    "    time elapsed since a given file's modification.\n"
    "\n"
    "    The -z argument takes a timezone string of the form used by the _tzset\n"
    "    function.  If no timezone is specified, the system local time is used.\n"
    "    The timezone also be set in the environment via the TZ environment\n"
    "    variable.  The format of this string is \"tzn[+|-]hh[:mm[:ss]][dzn]\", where\n"
    "\n"
    "        tzn\n"
    "            Three-letter time-zone name, such as PST.  You must specify the\n"
    "            correct offset from local time to UTC.\n"
    "\n"
    "        hh\n"
    "            Difference in hours between UTC and local time.  Optionally signed.\n"
    "\n"
    "        mm\n"
    "            Minutes.  Separated by hh by a colon (:).\n"
    "\n"
    "        ss\n"
    "            Seconds.  Separated by mm by a colon (:).\n"
    "\n"
    "        dzn\n"
    "            Three-letter daylight-saving-time zone such as PDT.  If daylight\n"
    "            saving time is never in effect in the locality, omit dzn.  The C\n"
    "            run-time library assumes the US rules for implementing the\n"
    "            calculation of Daylight Saving Time (DST).\n"
    "\n"
    "        Examples of the timezone string include the following:\n"
    "\n"
    "            UTC       Universal Coordinated Time\n"
    "            PST8      Pacific Standard Time\n"
    "            PST8PDT   Pacific Standard Time, daylight savings in effect.\n"
    "            GST-1GDT  German Standard Time, daylight savings in effect.\n"
    "\n"
    "    If no output string is supplied, the format specified in the environment\n"
    "    variable TIMEFORMAT is used.  If this variable is not set, then the format\n"
    "    defaults to \"%#c\".\n"
    "\n"
    "    Note that if your format string begins with - or /, you will need to prefix\n"
    "    it with a \\ character so that it is not confused with a command switch.\n"
    "\n"
    "    Strings take both \\ escape codes and %-arguments in the style of printf.\n"
    "    The \\ escape codes include \\n (newline), \\t (tab), \\b (backspace),\n"
    "    \\r (carriage return), and \\a (alert, or beep).\n"
    "\n"
    "    The %-arguments are\n"
    "\n"
    "        %%     Percent sign\n"
    "        %a     Abbreviated weekday name\n"
    "        %A     Full weekday name\n"
    "        %b     Abbreviated month name\n"
    "        %B     Full month name\n"
    "        %c     Date and time representation appropriate for locale\n"
    "        %d     Day of month as decimal number (01 - 31)\n"
    "        %D     Total elapsed whole days\n"
    "        %H     Hour in 24-hour format (00 - 23)\n"
    "        %I     Hour in 12-hour format (01 - 12)\n"
    "        %j     Day of year as decimal number (001 - 366)\n"
    "        %m     Month as decimal number (01 - 12)\n"
    "        %M     Minute as decimal number (00 - 59)\n"
    "        %p     Current locales A.M./P.M. indicator for 12-hour clock\n"
    "        %R     Total elapsed whole hours\n"
    "        %s     Total elapsed seconds\n"
    "        %S     Seconds as a decimal number (00 - 59)\n"
    "        %U     Week of year as number, Sunday as first day of week (00 - 51)\n"
    "        %w     Weekday as decimal number (0 - 6; Sunday is 0)\n"
    "        %W     Week of year, decimal, Monday as first day of week (00 - 51)\n"
    "        %x     Date representation for current locale\n"
    "        %X     Time representation for current locale\n"
    "        %y     Year without century, as decimal number (00 - 99)\n"
    "        %Y     Year with century, as decimal number\n"
    "        %z,%Z  Time-zone name or abbreviation; nil if time zone is unknown\n"
    "\n"
    "    As in the printf function, the # flag may prefix any formatting code.  In\n"
    "    that case, the meaning of the format code is changed as follows.\n"
    "\n"
    "        %#a, %#A, %#b, %#B, %#D, %#p, %#R, %#X, %#z, %#Z, %#%\n"
    "            The flag is ignored.\n"
    "\n"
    "        %#c\n"
    "            Long date and time representation, appropriate for current locale.\n"
    "            For example: Tuesday, March 14, 1995, 12:41:29.\n"
    "\n"
    "        %#x\n"
    "            Long date representation, appropriate to current locale.\n"
    "            For example: Tuesday, March 14, 1995.\n"
    "\n"
    "        %#d, %#H, %#I, %#j, %#m, %#M, %#S, %#U, %#w, %#W, %#y, %#Y\n"
    "            Remove any leading zeros.\n"
    "\n"
    "    Examples:\n"
    "\n"
    "        > currtime\n"
    "        Sunday, July 20, 2003 17:02:39\n"
    "\n"
    "        > currtime %H:%M:%S\n"
    "        17:03:17\n"
    "\n"
    "        > currtime -z UTC\n"
    "        Monday, July 21, 2003 00:03:47\n"
    "\n"
    "        > currtime Building endzones [%Y-%m-%d %#I:%M:%S %p].\n"
    "        Building endzones [2003-07-20 5:06:09 PM].\n"
    "\n"
    "        > echo. >timestamp.txt\n"
    "\n"
    "        [about a day and a half later...]\n"
    "\n"
    "        > currtime -m timestamp.txt Elapsed Time: %Dd, %H:%M:%S\n"
    "        Elapsed Time: 1d, 12:03:47\n"
    "        > currtime -m timestamp.txt Elapsed Time: %R:%M:%S\n"
    "        Elapsed Time: 36:03:47\n"
    "        > currtime -m timestamp.txt Elapsed Time: %s seconds\n"
    "        Elapsed Time: 129827 seconds\n"
    "\n"
    ;



/****************************************************************************/

const auto formatSize = 2048;             // Maximum Size of Format String

int main (int argc, char *argv[])
{
    time_t     longTime;
    time_t     offsetBase = -1;            // Offset Base Time
    char*      offsetBaseFile = nullptr;   // Offset Base File
    struct tm  currTime;                   // Current time
    char*      fmtptr;                     // Pointer into Format String
    char*      zone = nullptr;             // Time Zone;
    auto       escchar = '%';              // Escape Character
    char       format[formatSize] = "";    // Format String Buffer


    // Gather command-line arguments.

    for (auto i=1;  i < argc;  ++i) {
        auto argptr = argv[i];

        if (!((argv[i][0] == '-') || (argv[i][0] == '/'))) {
            if (format[0] == 0)
                strcpy_s (format, formatSize, argptr);
            else {
                strcat_s (format, formatSize, " ");
                strcat_s (format, formatSize, argptr);
            }
        } else {
            auto optchar = argv[i][1];     // Option Character

            if (optchar == 0) {
                fputs ("Error: Null option switch.\n", stderr);
                return -1;
            }

            // Point argptr to the contents of the switch.  This may be immediately following the
            // option character, or it may be the next token on the command line.

            if (argv[i][2] == 0) {
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

            switch (optchar) {
                default:
                    fprintf (stderr, "Error: Unrecognized option (-%c).\n", optchar);
                    return -1;

                // Alternate Escape Character

                case 'e':
                    if (argptr) escchar = *argptr;
                    break;

                // Command Help

                case 'H':
                case 'h':
                case '?':
                    fputs (usage, stdout);
                    return 0;

                // Modification Base Time

                case 'm':
                    offsetBaseFile = argptr;
                    break;

                // Timezone

                case 'z':
                    zone = argptr;
                    break;
            }

            // Currently, all options take an argument, so flag an error if we're missing
            // an argument.

            if (argptr == 0) {
                fprintf (stderr, "Error: Missing argument for -%c switch.\n", optchar);
                return -1;
            }
        }
    }

    // If no format string was specified on the command line, fetch it from the TIMEFORMAT
    // environment variable.  If not available there, then use the default format string.

    if (format[0] == 0) {
        const auto timeFormatName = "TIMEFORMAT";
        size_t buffSize;
        getenv_s (&buffSize, nullptr, 0, timeFormatName);

        if (buffSize == 0) {
            strcpy_s (format, formatSize, "%#c");
        } else {   auto buffer = new char[buffSize];
            getenv_s (&buffSize, buffer, buffSize, timeFormatName);
            strcpy_s (format, formatSize, buffer);
            delete buffer;
        }
    }

    // If an alternate time zone was specified, then we need to set the TZ environment variable.
    // Kludgey, but I couldn't find another way.

    if (zone) {
        const auto buffSize = 1 + strlen("TZ=") + strlen(zone);
        auto buff = new char [buffSize];

        if (buff == 0) {
            fputs ("Error: Out of memory.\n", stderr);
            return -1;
        }

        strcpy_s (buff, buffSize, "TZ=");
        strcat_s (buff, buffSize, zone);

        _putenv (buff);

        delete buff;
    }

    // If an offset base file was specified, get the modification time from the file.

    if (offsetBaseFile) {
        struct _stat stat;    // File Status Data

        if (0 != _stat(offsetBaseFile, &stat)) {
            fprintf (stderr, "Couldn't get status of \"%s\".\n", offsetBaseFile);
            return -1;
        }

        offsetBase = stat.st_mtime;
    }

    // Get the current time. If an offset file was specified, subtract that
    // file's modification time from the current time.

    time (&longTime);

    if (offsetBaseFile) {
        if (longTime < offsetBase) {
            fprintf (stderr, "currtime: Time zone error: is your environment variable TZ set incorrectly?\n");
            return -1;
        }

        longTime -= offsetBase;
        gmtime_s (&currTime, &longTime);

    } else {

        localtime_s (&currTime, &longTime);
    }

    // Now scan through the format string, emitting expanded characters along the way.

    for (fmtptr=format;  *fmtptr;  ++fmtptr) {
        const auto buffsize = 1024;
        char buff [buffsize];   // Intermediate Output Buffer

        // Handle backslash sequences, unless backslash is the alternate escape character.

        if ((*fmtptr == '\\') && (escchar != '\\')) {
            ++fmtptr;

            switch (*fmtptr) {
                // Unrecognized \-sequences resolve to the escaped character.

                default:   putchar(*fmtptr);  break;

                // If the string ends with a \, then just emit the \.

                case 0:    putchar('\\');  break;

                // Recognized \-sequences are handled here.

                case 'n':  putchar('\n');  break;
                case 't':  putchar('\t');  break;
                case 'b':  putchar('\b');  break;
                case 'r':  putchar('\r');  break;
                case 'a':  putchar('\a');  break;
            }

        } else if (*fmtptr == escchar) {

            char token[4];    // Escape Token Word

            ++fmtptr;

            switch (*fmtptr) {
                case 'D':   // Total Elapsed Whole Days
                    printf ("%I64d", longTime / (24*60*60));
                    break;

                case 'R':  // Total Elapsed Whole Hours
                    printf ("%I64d", longTime / (60*60));
                    break;

                case 's':  // Total Elapsed Seconds
                    printf ("%I64d", longTime);
                    break;

                default:
                    // All other escape sequences are handled here.

                    token[0] = '%';
                    token[1] = *fmtptr;
                    token[2] = 0;

                    if (*fmtptr == '#') {
                        token[2] = *++fmtptr;
                        token[3] = 0;
                    }

                    strftime (buff, buffsize, token, &currTime);
                    fputs (buff, stdout);

                    break;
            }

        } else {

            // All unescaped character are emitted as-is.

            putchar (*fmtptr);
        }
    }

    // Print the final newline.

    putchar ('\n');
    return 0;
}