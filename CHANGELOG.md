Change Log -- `timeprint`
================================================================================

v3.0.0  (In Progress)
----------------------------------------------------------------------------------------------------
### Major Changes
  - Delta time output now checks environment variable `TimeFormat_Delta` instead of old `TimeFormat`
    (#45).
  - New default output format for printing time differences / elapsed times (#44)
  - Project now builds using CMake (#52)

### Minor Changes
  - Updated help output.
  - Unrecognized options (including `-`) are now just accrued to the format string. Beware
    possible collision with future options.

### Patches


v2.1.0  (2018-05-24)
----------------------------------------------------------------------------------------------------
### Minor Changes
  - Indicate unknown DST status for calls to `mktime()`.
  - Print error message for unhandled explicit dates before 1970.
  - New: Handle numeric prefix on `%a` format codes.


v2.0.0  (2018-02-26)
----------------------------------------------------------------------------------------------------
### Major Changes
  - Deprecate option `--modTime`
  - -e is now -%/--codeChar to eliminate confusion between escape sequences and
    format code sequences.
  - Renamed from currtime to timeprint.
  - Moved custom elapse codes to use new underscore modifier.
    + Old `%D` is replaced with `%_D`,
      `%D` is now standard "Short MM/DD/YY date, equivalent to %m/%d/%y".
    + Old `%R` is replaced with `%_H`,
      `%R` is now standard "24-hour HH:MM time, equivalent to %H:%M".
    + Old `%s` is replaced with `%_S`, `%s` is now undefined.

### Minor Changes
  - Add new `--` switch variants (`--help`, `--codeChar`, `--timeZone`, and so on).
  - Add help information on new (C++11) format codes.
  - New rich suite of delta time format options.
  - New options `--creation`, `--access`, `--modification`.
  - New option `--now`
  - New option `--time`
  - Help output broken into multiple topics.

### Patches
  - Fix: crashing bug when the format contained unrecognized `%`-codes.
  - Fix: crashing bug on missing `--modTime`, `--timeZone` options.


v1.0.0  (original 2013-03-09, released 2018-02-15)
----------------------------------------------------------------------------------------------------
  - Original version snap as of 2013-03-09.
  - Added license
  - First release to GitHub
