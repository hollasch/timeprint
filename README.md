`timeprint`
================================================================================
Print time and date information.


Summary
-------------
`timeprint` is a Windows command-line tool that provides a way to format and
display aspects of the current or elapsed date and time. It is based on the
C++ standard `strftime` function, with some additional functionality.

For full documentation on `timeprint`, see [timeprint.md](./timeprint.md).


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
