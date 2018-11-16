@echo off & setlocal
: ==================================================================================================
: Acceptance testing for `timeprint`. Note that to properly pass % characters, they must be escaped
: (sometimes twice).
:
: NOTE: This test requires the `diff` tool to be on your path.
: ==================================================================================================

set testOut=out

if "%1" equ "--run" goto :runTests

if "%1" neq "" (
    set timePrint=out\%1\timeprint.exe
) else (
    set timePrint=out\Debug\timeprint.exe
)

echo Testing %timePrint%.

if not exist %timePrint% (
    echo ERROR: Executable %timePrint% not found. 1>&2
    exit /b 1
)

call %0 --run %timePrint% > %testOut%\tests-output.txt
diff -u3 tests-accepted.txt %testOut%\tests-output.txt

if %errorlevel% equ 0 (
    echo All tests pass.
    goto :eof
)

echo.
set /p response="Differences found. Accept new results? "
if /i "%response%" neq "y"  if /i "%response%" neq "yes"  exit /b 1

copy %testOut%\tests-output.txt tests-accepted.txt
exit /b 0



: ==================================================================================================
:runTests

set timePrint=%2
set testNum=1

echo Acceptance Tests for `timeprint`
echo ================================================================================
echo.

echo.--------------------------------------------------------------------------------
echo Test %testNum%: [/?]
%timePrint% /?
set /a testNum = testNum + 1

call :test --help FORMATCODES
call :test -H timeSyntax
call :test /htimezone
call :test --help examples
call :test --help deltaTime

set TIMEFORMAT=Test TIMEFORMAT environment variable.
call :test
set TIMEFORMAT=

call :testCapture general-help --help
call :testEqual   general-help -h bogusHelpTopic
call :testEqual   general-help -hbogusHelpTopic
call :testEqual   general-help --help bogusHelpTopic

call :errTest --
call :errTest -
call :errTest --bogusSwitch
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
call :test "A%%%%nB%%%%nC"
call :test "A%%%%tB%%%%tC"
call :test Percent sign = %%%%%%%%
call :test Bogus codes: %%%%E%%%%f%%%%i%%%%J%%%%N%%%%P%%%%s%%%%v
call :test Bogus codes: %%%%_a %%%%_z

call :test --timezone UTC --time 2000-01-01T00:00:00Z
call :test --timezone UTC --time 2000-01-02T03:04:05+67
call :test --timezone UTC --time 2000-01-02T03:04:05-67:89
call :test --timezone UTC --time 2000-01-02T03:04:05-6789
call :test --timezone UTC --time 2000-01-01T12:00Z "%%%%1a %%%%2a %%%%3a %%%%4a %%%%5a %%%%6a %%%%7a %%%%8a %%%%9a %%%%20a"
call :test --timezone PST+08 --time 2000-01-01T00:00:00Z "%%%%#c %%%%z %%%%Z"

call :test --time 2000-01-01T00:00:00Z --time 2000-01-02T00:00:00Z "%%%%_S"
call :test --time 2000-01-01T00:00:00Z --time 2000-01-02T03:04:05Z "%%%%_Dd %%%%_dH:%%%%_hM:%%%%_mS"
call :test --time 2000-01-01T00:00:00Z --time 2000-01-02T03:04:05Z "%%%%_Dd %%%%_d0H:%%%%_h0M:%%%%_m0S"
call :test --time 2000-01-01T00:00:00Z --time 2000-01-02T03:04:05Z "%%%%_D."
call :test --time 2000-01-01T00:00:00Z --time 2000-01-02T03:04:05Z "%%%%_D.8"

call :test --now --creation timeprint.cpp "%%%%_"
call :test --now --creation timeprint.cpp "%%%%_y"
call :test --now --creation timeprint.cpp "%%%%_y."
call :test --now --creation timeprint.cpp "%%%%_yy (bogus delta time value)"
call :test --now --creation timeprint.cpp "%%%%_tt (bogus delta time value)"
call :test --now --creation timeprint.cpp "%%%%_xy (bogus delta time modulo unit type)"
call :test --now --creation timeprint.cpp "%%%%_xt (bogus delta time modulo unit type)"
call :test --now --creation timeprint.cpp "%%%%_xd (bogus delta time modulo unit type)"
call :test --now --creation timeprint.cpp "%%%%_xh (bogus delta time modulo unit type)"
call :test --now --creation timeprint.cpp "%%%%_xm (bogus delta time modulo unit type)"
call :test --now --creation timeprint.cpp "%%%%_'yM.0 (spurious delta time lead character)"



echo.--------------------------------------------------------------------------------
exit /b 0




:test
    echo.--------------------------------------------------------------------------------
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
