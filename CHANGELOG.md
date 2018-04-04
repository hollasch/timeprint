Change Log for `timeprint`
================================================================================

## v2.0.0+ (In Progress)
  - Indicate unknown DST status for calls to mktime().
  - Print error message for unhandled explicit dates before 1970.

## v2.0.0 (2018-02-26)
  - Renamed from currtime to timeprint.
  - Add new -- switch variants (--help, --codeChar, --timeZone, and so on).
  - -e is now -%/--codeChar to eliminate confusion between escape sequences and
    format code sequences.
  - Add help information on new (C++11) format codes.
  - Moved custom elapse codes to use new underscore modifier.
    + Old %D is replaced with %_D,
      %D is now standard "Short MM/DD/YY date, equivalent to %m/%d/%y".
    + Old %R is replaced with %_H,
      %R is now standard "24-hour HH:MM time, equivalent to %H:%M".
    + Old %s is replaced with %_S, %s is now undefined.
  - New rich suite of delta time format options.
  - Fix crashing bug when the format contained unrecognized %-codes.
  - Fix crashing bug on missing --modTime, --timeZone options.
  - New options `--creation`, `--access`, `--modification`.
  - New option `--now`
  - Deprecate option `--modTime`
  - New option `--time`
  - Help output broken into multiple topics.


## v1.0.0  (original 2013-03-09, released 2018-02-15)
  - Original version snap as of 2013-03-09.
  - Added license
  - First release to GitHub
