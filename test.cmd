@echo off & setlocal
:: =================================================================================================
:: Acceptance testing for `timeprint`. Note that to properly pass % characters, they must be escaped
:: (sometimes twice).
::
:: NOTE: This test requires the `diff` tool to be on your path.
::
:: Usage: test [release|debug]
::
:: By default, tests the release build.
::
:: =================================================================================================

set testOut=out

:: This script sets up runtime parameters and then calls itself with the `--run` argument to capture
:: test output and report results. You can find the :runTests label after the equals line below.
if "%1" equ "run" goto :runTests

if not exist %testOut% mkdir %testOut%

set testScript=%0

set interactive=1
if "%1" equ "--non-interactive" (
    set interactive=0
    shift
)

if "%1" equ "release" (
    set buildConfig=release
) else if "%1" equ "" (
    set buildConfig=release
) else if "%1" equ "debug" (
    set buildConfig=debug
) else (
    echo 1>&2ERROR: Invalid build configuation ^(%1^). Expected "release" or "debug".
    exit /b 1
)

if %buildConfig% equ release (
    call cmake --build build --config release
    set timePrint=build\Release\timeprint.exe
) else (
    call cmake --build build --config debug
    set timePrint=build\Debug\timeprint.exe
)

echo.
echo Testing %timePrint%.

if not exist %timePrint% (
    echo ERROR: Executable %timePrint% not found. 1>&2
    exit /b 1
)

call %testScript% run %timePrint% > %testOut%\tests-output.txt
diff -u3 tests-accepted.txt %testOut%\tests-output.txt

if %errorlevel% equ 0 (
    echo PASS: All output matches.
    goto :pass
)

echo.
if %interactive% equ 0 (
    echo FAIL: Output changed.
    goto :fail
)

set /p response="Differences found. Accept new results (yes/no/manual)? "

if /i "%response%" equ "n"      goto :failUnaccepted
if /i "%response%" equ "no"     goto :failUnaccepted

if /i "%response%" equ "y"      goto :failAcceptAll
if /i "%response%" equ "yes"    goto :failAcceptAll

if /i "%response%" equ "m"      goto :failAcceptManual
if /i "%response%" equ "merge"  goto :failAcceptManual
if /i "%response%" equ "manual" goto :failAcceptManual

goto :failUnaccepted

:failAcceptManual
    if not defined MERGETOOL (
        echo 1>&2ERROR: Cannot merge; environment variable MERGETOOL is not defined.
        goto :fail
    )
    call "%MERGETOOL%" %testOut%\tests-output.txt tests-accepted.txt
    goto :pass

:failAcceptAll
    copy %testOut%\tests-output.txt tests-accepted.txt
    echo PASS: New output accepted entirely.
    goto :pass

:failUnaccepted
    echo FAIL: Output changed, differences unaccepted.
    goto :fail

:fail
    echo.
    exit /b 1

:pass
    echo.
    exit /b 0


::==================================================================================================
:runTests

    set timePrint=%2
    set testNum=1

    echo Acceptance Tests for `timeprint`
    echo ================================================================================
    echo.

    echo.--------------------------------------------------------------------------------
    echo Test %testNum%: [/?]
    %timePrint% /? | findstr -v github.com/hollasch/timeprint
    set /a testNum = testNum + 1

    call :test --help examples
    call :test --help deltaTime
    call :test --help FORMATCODES
    call :test -H timeSyntax
    call :test -htimezone
    call :test /hexamples

    set comment="User time format from env var"
    set TIMEFORMAT=Test TIMEFORMAT environment variable.
    call :test
    set TIMEFORMAT=

    set comment="User time delta format from env var"
    set TIMEFORMAT_DELTA=Test TIMEFORMAT_DELTA environment variable.
    call :testDefCode --time 08:00 --time 15:00
    call :test --time 08:00 --time 15:00

    set comment="Default time delta format"
    set TIMEFORMAT_DELTA=
    call :testDefCode --time 2000-01-01T00:00:00 --time 2018-11-16T15:57:05
    call :test --time 2000-01-01T00:00:00 --time 2018-11-16T15:57:05

    call :testCapture general-help --help
    call :testEqual   general-help -h bogusHelpTopic
    call :testEqual   general-help -hbogusHelpTopic
    call :testEqual   general-help --help bogusHelpTopic

    call :test --
    call :test -
    call :test --bogusSwitch

    call :errTest -a
    call :errTest --access
    call :errTest --access someBogusFile
    call :errTest -c
    call :errTest --creation
    call :errTest --creation someBogusFile
    call :errTest -m
    call :errTest --modification
    call :errTest --modification someBogusFile
    call :errTest --time
    call :errTest --time 12:00 --access file1 --modification file2
    call :errTest --now --access file1 --modification file2
    call :errTest --access file1 --modification file2 --now
    call :errTest --modification file2 --now --time 12:00
    call :errTest -z
    call :errTest --timezone

    call :test A b c d e Hello world f g h i j
    call :test "A b c d e Hello world f g h i j"
    call :test "A\nB\nC"
    call :test "A\tB\tC"

    call :testDefCode "A%%%%nB%%%%nC"
    call :testDefCode "A%%%%tB%%%%tC"
    call :testDefCode Percent sign = %%%%%%%%
    call :testDefCode --codeChar $ --time 2000-01-02T03:04:05 $Y $m $d $H $M $S

    call :test "A$nB$nC"
    call :test "A$tB$tC"
    call :test "Dollar sign = $$"

    call :test Bogus codes: $E $f $i $J $N $P $s $v
    call :test Bogus codes: $_a $_z

    call :test --timezone UTC --time 2000-01-01T00:00:00Z
    call :test --timezone UTC --time 2000-01-02T03:04:05+67
    call :test --timezone UTC --time 2000-01-02T03:04:05-67:89
    call :test --timezone UTC --time 2000-01-02T03:04:05-6789
    call :test --timezone UTC --time 2000-01-01T12:00Z "$1a $2a $3a $4a $5a $6a $7a $8a $9a $20a"
    call :test --timezone PST+08 --time 2000-01-01T00:00:00Z "$#c $z $Z"

    call :test --time 2000-01-01T00:00:00Z --time 2000-01-02T00:00:00Z "$_S"
    call :test --time 2000-01-01T00:00:00Z --time 2000-01-02T03:04:05Z "$_Dd $_dH:$_hM:$_mS"
    call :test --time 2000-01-01T00:00:00Z --time 2000-01-02T03:04:05Z "$_Dd $_d0H:$_h0M:$_m0S"
    call :test --time 2000-01-01T00:00:00Z --time 2000-01-02T03:04:05Z "$_D."
    call :test --time 2000-01-01T00:00:00Z --time 2000-01-02T03:04:05Z "$_D.8"

    call :test --time 2000-01-01T00:00:00Z --time 2002-05-07T09:07:53Z "$_M.4"
    call :test --time 2000-01-01T00:00:00Z --time 2002-05-07T09:07:53Z "$_'|_M.4"
    call :test --time 2000-01-01T00:00:00Z --time 2002-05-07T09:07:53Z "$_'0_M.4"

    call :test --now --creation timeprint.cpp "$_ (bogus delta time value)"
    call :test --now --creation timeprint.cpp "$_y (bogus delta time value)"
    call :test --now --creation timeprint.cpp "$_y. (bogus delta time value)"
    call :test --now --creation timeprint.cpp "$_yy (bogus delta time value)"
    call :test --now --creation timeprint.cpp "$_tt (bogus delta time value)"
    call :test --now --creation timeprint.cpp "$_xy (bogus delta time modulo unit type)"
    call :test --now --creation timeprint.cpp "$_xt (bogus delta time modulo unit type)"
    call :test --now --creation timeprint.cpp "$_xd (bogus delta time modulo unit type)"
    call :test --now --creation timeprint.cpp "$_xh (bogus delta time modulo unit type)"
    call :test --now --creation timeprint.cpp "$_xm (bogus delta time modulo unit type)"
    call :test --now --creation timeprint.cpp "$_'yM.0 (spurious delta time lead character)"

    echo.--------------------------------------------------------------------------------
    exit /b 0


:test
    echo.--------------------------------------------------------------------------------
    if defined comment echo %comment%
    set comment=
    echo [--codeChar $ %*]
    %timePrint% --codeChar $ %*
    set /a testNum = testNum + 1
    goto :eof


:testDefCode
    echo.--------------------------------------------------------------------------------
    if defined comment echo %comment%
    set comment=
    echo [%*]
    %timePrint% %*
    set /a testNum = testNum + 1
    goto :eof


:testCapture
    %timePrint% %2 %3 %4 %5 %6 %7 %8 %9 > %testOut%\test-output-%1.txt
    goto :eof


:testEqual
    echo.--------------------------------------------------------------------------------
    echo Output [%2 %3 %4 %5 %6 %7 %8 %9] equal to %1
    %timePrint% %2 %3 %4 %5 %6 %7 %8 %9 > %testOut%\test-output-%testNum%.txt
    fc >nul %testOut%\test-output-%testNum%.txt %testOut%\test-output-%1.txt
    if %ERRORLEVEL% equ 0 (
        echo Test passed.
    ) else (
        echo Test failed.
    )
    set /a testNum = testNum + 1
    goto :eof
    

:errTest
    echo.--------------------------------------------------------------------------------
    echo Error Test [%*]
    %timePrint% %* 1>nul 2>%TEMP%\timeprint-test-error-output.txt
    type %TEMP%\timeprint-test-error-output.txt
    del  %TEMP%\timeprint-test-error-output.txt
    set /a testNum = testNum + 1
    goto :eof
