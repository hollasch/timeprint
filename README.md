timeprint
====================================================================================================

Print time and date information.


Summary
--------
`timeprint` is a Windows command-line tool that provides a way to format and display aspects of the
current or elapsed date and time. It is based on the C++ standard `strftime` function, plus
additional functionality.

For full documentation on `timeprint`, see [timeprint.md](./timeprint.md).


Installation
-------------
The built executable is `timeprint.exe`, and can be copied anywhere to your command path. There is
no Windows installation required for this tool.


Building
----------
This project uses the CMake build tool. CMake is a meta-build system that locates and uses your
local development tools to build the project if possible.

To build, first install [CMake][https://cmake.org/]. Then go to the project root directory and run
the following command:

    cmake -B build

This will locate your installed development tools and configure your project build in the `build/`
directory. After that, whenever you want a new build, run this command:

    cmake --build build

This will build a debug version of the project, located in `build/Debug/`. To build a release
version, run

    cmake --build build --config release

You can find the built release executable in `build/Release/`.


Testing
--------
`timeprint` is validated through acceptance testing. Basically, it's run against a suite of inputs,
and output is compared with known good prior results. Differences are reported, and either expose a
regression or bug, or new proper behavior. In the case of a validated new set of outputs, the
known-good output is updated.

To perform a test, run `test.cmd` from the command line at the root of this project. This tool
requires that you have `diff.exe` on your execution path.


--------------------------------------------------------------------------------
Steve Hollasch, steve@hollasch.net<br>
https://github.com/hollasch/timeprint
