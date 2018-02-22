`timeprint`
================================================================================
Print time and date information.


Summary
-------------
`timeprint` is a Windows command-line tool that provides a way to format and
display aspects of the current or elapsed date and time. It is based on the
C++ standard `strftime` function, with some additional functionality.

- [Usage](#usage)
- [Description](#description)
- [Time Zones](#time-zones)
- [Format Codes](#format-codes)
- [Time Syntax](#time-syntax)
- [Examples](#examples)
- [Building](#building)
- [Installation](#installation)
- [Testing](#testing)



Usage
-------
    timeprint [--codeChar <char>] [-%<char>]
              [--help [topic]] [-h[topic]] [/?]
              [--access <fileName>] [-a<fileName>]
              [--creation <fileName>] [-c<fileName>]
              [--modification <fileName>] [-m<fileName>]
              [--timeZone <zone>] [-z<zone>]
              [--now] [-n]
              [--time <timeValue>] [-t<timeValue>]
              [string] ... [string]


Description
-------------
This command prints time information to the standard output stream. All string
fragments will be concatenated with a space, so it's usually unnecessary to
quote the format string.

timeprint operates in either absolute or differential mode. If one time value
is specified, then values for that absolute time are reported. If two time
values are supplied, then timeprint reports the values for the positive
difference between those two values. If no time values are given, then `--now`
is implied.

Command switches may be prefixed with a dash (`-`) or a slash (`/`).

`--access <fileName>`, `-a<fileName>`<br>
Use the time of last access of the named file for a time value.

`--codeChar <char>`, `-%<char>`<br>
The --codeChar switch specifies an alternate code character to the
default `%` character. If the backslash (`\`) is specified as the code
character, then normal backslash escapes will be disabled. The
`--codeChar` switch is ignored unless the format string is specified on
the command line.

`--creation <fileName>`, `-c<fileName>`<br>
Use the creation time of the named file.

`--help [topic]`, `-h[topic]`, `-?[topic]`<br>
Print help and usage information in general, or for the specified
topic. Topics include 'examples', 'formatCodes', 'timeSyntax', and
'timezone'.

`--modification <fileName>`, `-m<fileName>`<br>
Use the modification time of the named file.

`--now`, `-n`<br>
Use the current time.

`--time <value>`, `-t<value>`<br>
Specifies an explicit absolute time, using ISO 8601 syntax. For a
description of supported syntax, use `--help timeSyntax`.

`--timeZone <zone>`, `-z<zone>`<br>
The `--timeZone` argument takes a timezone string of the form used by
the `_tzset()` function. If no timezone is specified, the system local
time is used. The timezone can be set in the environment via the `TZ`
environment variable. For a description of the time zone format, use
`--help timeZone`.

If no output string is supplied, the format specified in the environment
variable `TIMEFORMAT` is used. If this variable is not set, then the format
defaults to `%#c`.

Note that if your format string begins with `-` or `/`, you will need to prefix
it with a `\` character so that it is not confused with a command switch.

Strings take both \-escaped characters and %-codes in the style of printf.
The escape codes include `\n` (newline), `\t` (tab), `\b` (backspace),
`\r` (carriage return), and `\a` (alert, or beep).

For a full description of supported format codes, use `--help formatCodes`.

For additional help, use `--help <topic>`, where <topic> is one of

  - `examples`
  - `formatCodes`
  - `timeSyntax`
  - `timeZone`


Time Zones
------------
Time zones have the format `tzn[+|-]hh[:mm[:ss]][dzn]`, where

`tzn` — Three-letter time-zone name, such as PST. You must specify the correct
offset from local time to UTC.

`hh` — Difference in hours between UTC and local time. Optionally signed.

`mm` — Minutes, separated with a colon (:).

`ss` — Seconds, separated with a colon (:).

`dzn` — Three-letter daylight-saving-time zone such as PDT. If daylight saving
time is never in effect in the locality, omit dzn. The C run-time library
assumes the US rules for implementing the calculation of Daylight Saving Time
(DST).

Examples of the timezone string include the following:

| Zone       | Meaning
|:-----------|:--------------------------------------------------
| `UTC`      | Universal Coordinated Time
| `PST8`     | Pacific Standard Time
| `PST8PDT`  | Pacific Standard Time, daylight savings in effect
| `GST-1GDT` | German Standard Time, daylight savings in effect

Format Codes
--------------
The following time format codes are supported:


| Code  | Result
|-------|-----------------------------------------------------------------------
| `%a`  | Abbreviated weekday name *
| `%A`  | Full weekday name *
| `%b`  | Abbreviated month name *
| `%B`  | Full month name *
| `%c`  | Date and time representation *
| `%C`  | Year divided by 100 and truncated to integer (00-99)
| `%d`  | Day of month as decimal number (01-31)
| `%D`  | Short MM/DD/YY date, equivalent to %m/%d/%y
| `%e`  | Day of the month, space-padded ( 1-31)
| `%F`  | Short YYYY-MM-DD date, equivalent to %Y-%m-%d
| `%g`  | Week-based year, last two digits (00-99)
| `%G`  | Week-based year
| `%h`  | Abbreviated month name (same as %b) *
| `%H`  | Hour in 24-hour format (00-23)
| `%I`  | Hour in 12-hour format (01-12)
| `%j`  | Day of year as decimal number (001-366)
| `%m`  | Month as decimal number (01-12)
| `%M`  | Minute as decimal number (00-59)
| `%n`  | New line character (same as '\n')
| `%p`  | AM or PM designation
| `%r`  | 12-hour clock time *
| `%R`  | 24-hour HH:MM time, equivalent to %H:%M
| `%S`  | Seconds as a decimal number (00-59)
| `%t`  | Horizontal tab character (same as '\t')
| `%T`  | ISO 8601 time format (HH:MM:SS) equivalent to %H:%M:%S
| `%u`  | ISO 8601 weekday as number with Monday=1 (1-7)
| `%U`  | Week number, first Sunday = week 1 day 1 (00-53)
| `%V`  | ISO 8601 week number (01-53)
| `%w`  | Weekday as decimal number, Sunday = 0 (0-6)
| `%W`  | Week of year, decimal, Monday = week 1 day 1(00-51)
| `%x`  | Date representation *
| `%X`  | Time representation *
| `%y`  | Year without century, as decimal number (00-99)
| `%Y`  | Year with century, as decimal number
| `%z`  | ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100). If timezone cannot be determined, no characters
| `%Z`  | Time-zone name or abbreviation, empty for unrecognized zones *
| `%_S` | Elapsed whole seconds
| `%%`  | Percent sign

\* _Specifiers marked with an asterisk are locale-dependent._

As in the printf function, the `#` flag may prefix any formatting code. In that
case, the meaning of the format code is changed as follows.

`%#c`<br>
Long date and time representation, appropriate for current locale.
For example: Tuesday, March 14, 1995, 12:41:29.

`%#x`<br>
Long date representation, appropriate to current locale. For example:
Tuesday, March 14, 1995.

`%#d`, `%#H`, `%#I`, `%#j`, `%#m`, `%#M`, `%#S`, `%#U`, `%#w`, `%#W`, `%#y`, `%#Y`<br>
Remove any leading zeros.

**All others**<br>
The flag is ignored.


Time Syntax
-------------
The explicit '--time' option supports a variety of different formats, using the
ISO 8601 date/time format.

(_To be completed._)


Examples
----------
```
> timeprint
Sunday, July 20, 2003 17:02:39

> timeprint %H:%M:%S
17:03:17

> timeprint -z UTC
Monday, July 21, 2003 00:03:47

> timeprint Building levels [%Y-%m-%d %#I:%M:%S %p].
Building levels [2003-07-20 5:06:09 PM].

> echo. >timestamp.txt
[a day and a half later...]
> timeprint --modification timestamp.txt --now Elapsed Time: %_S seconds
Elapsed Time: 129600 seconds
```


Installation
--------------
The built executable is `timeprint.exe`, and can be copied anywhere to your
command path. There is no Windows installation required for this tool.


Building
----------
This tool is built with Microsoft Visual Studio Community Edition. The source is
a single C++ file, so it should be trivial to build with any other toolset.


Testing
---------
`timeprint` is validated through acceptance testing. Basically, it's run against
a suite of inputs, and output is compared with known good prior results.
Differences are reported, and either expose a regression or bug, or new proper
behavior. In the case of a validated new set of outputs, the
known-good output is updated.

To perform a test, run `test.cmd` from the command line at the root of this
project. This tool requires that you have `diff.exe` on your execution path.


----------------------------------------------------------------------------------------------------
Steve Hollasch, steve@hollasch.net<br>
https://github.com/hollasch/timeprint
