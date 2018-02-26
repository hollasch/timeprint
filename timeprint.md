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
- [Delta Time Formatting](#delta-time-formatting)
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
The --timeZone argument takes a timezone string of the form used by
the TZ environment variable. If no timezone is specified, the value
in the TZ environment variable is used. If the environment variable
TZ is unset, the system local time is used. For a description of the
time zone format, use `--help timeZone`.\n"

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
The time zone value may be specified with the TZ environment variable,
or using the `--timezone` option. Time zones have the format
`tzn[+|-]hh[:mm[:ss]][dzn]`, where

`tzn` — Time-zone name, three letters or more, such as `PST`.

`[+|-]hh` — The time that must be ADDED to local time to get UTC.
*CAREFUL*: Unfortunately, this value is negated from how time zones
are normally specified. For example, PDT is specified as `-0800`,
but in the time zone string, will be specified as `PDT+08`.
You can experiment with the string \"%#c %Z %z\" and the
`--timezone` option to ensure you understand how these work
together. If offset hours are omitted, they are assumed to be
zero.

`[:mm]` — Minutes, prefixed with mandatory colon.

`[:ss]` — Seconds, prefixed with mandatory colon.

`[dzn]` — Three-letter daylight-saving-time zone such as PDT. If daylight saving
time is never in effect in the locality, omit dzn. The C run-time library
assumes the US rules for implementing the calculation of Daylight Saving Time
(DST).

Examples of the timezone string include the following:

| Zone        | Meaning
|:------------|:--------------------------------------------------
| `UTC`       | Universal Coordinated Time
| `PST8`      | Pacific Standard Time
| `PDT+07`    | Pacific Daylight Time
| `NST+03:30` | Newfoundland Standard Time
| `PST8PDT`   | Pacific Standard Time, daylight savings in effect
| `GST-1GDT`  | German Standard Time, daylight savings in effect


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


Delta Time Formatting
-----------------------
Time differences are reported using the delta time formats. The delta time
format has the following syntax:

                       %_['kd][u[0]]<U>[.[#]]
                          -v-  -v--  v  -v--
    Numeric Format --------'    |    |   |
    Next Greater Unit ----------'    |   |
    Units ---------------------------'   |
    Decimal Precision -------------------'

### Numeric Format `['kd]` (_optional_)
The optional `'` character is followed by two characters, k and d.
k represents the character to use for the thousand's separator, with
the special case that `0` indicates that there is to be no thousands
separator.

The d character is the character to use for the decimal point, if one
is present. So, for example, `'0.` specifies no thousands separator,
and the American `.` decimal point. `'.,` would specify European
formatting, with `.` for the thousands separator, and `,` as the
decimal point.

### Next Greater Unit `[u[0]]` (_optional_)
This single lowercase letter indicates any preceding units used in the
delta time printing. For example, if the unit is hours, and the next
greater unit is years, then the hours reported are the remainder
(modulo) after the number of years. Supported next greater units
include the following:

|Letter| Meaning
|:----:|:------------------------------------------------
| `y`  | Nominal years (see units below for definition)
| `t`  | Tropical years (see units below for definition)
| `d`  | Days
| `h`  | Hours
| `m`  | Minutes

If the next greater unit is followed by a zero, then the result is
zero-padded to the appropriate width for the range of possible values.

### Units `<U>` (_required_)
The unit of time (single uppercase letter) to report for the time
delta. This is the remainder after the (optional) next greater unit.
The following units are supported:

|Letter| Meaning
|:----:|:---------------
| `Y`  | Nominal years
| `T`  | Tropical years
| `D`  | Days
| `H`  | Hours
| `M`  | Minutes
| `S`  | Whole seconds

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

### Decimal Precision `[.[#]]` (_optional_)
With the exception of seconds, all units will have a fractional value
for time differences. If the decimal precision format is omitted, the
then rounded whole value is printed.

If the decimal point and number is specified, then the fractional
value will be printed with the number of requested digits.

If a decimal point is specified but without subsequent digits, then
the number of digits will depend on the units. Enough digits will be
printed to maintain full resolution of the unit to within one second.
Thus, years: 8 digits, days: 5, hours: 4, minutes: 2.

### Examples
Given a delta time of 547,991,463 seconds, the following delta format
strings will yield the following output:

    %_S
        "547991463"

    %_',.S
        "547,991,463"

    %_Y years, %_yD days, %_dH. hours
        "17 years, 137 days, 11.8508 hours"

See `--time examples` for more example uses of delta time formats.


Time Syntax
-------------
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
Elapsed Time: 129797 seconds
> timeprint --modification timestamp.txt --now Elapsed Time: %_H:%_hM:%_mS
Elapsed Time: 36:3:17
```


----------------------------------------------------------------------------------------------------
Steve Hollasch, steve@hollasch.net<br>
https://github.com/hollasch/timeprint
