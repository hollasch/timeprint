@echo off & setlocal
: ==================================================================================================
: Acceptance testing for `timeprint`. Note that to properly pass % characters, they must be escaped
: (sometimes twice).
:
: NOTE: This test requires the `diff` tool to be on your path.
: ==================================================================================================

set testOut=x64

if "%1" equ "--run" goto :runTests

if "%1" neq "" (
    set timePrint=x64\%1\timeprint.exe
) else (
    set timePrint=x64\Debug\timeprint.exe
)

if not exist %timePrint% (
    echo ERROR: Executable %timePrint% not found. 1>&2
    exit /b 1
)

call %0 --run %timePrint% > %testOut%\tests-output.txt
diff -u3 tests-accepted.txt %testOut%\tests-output.txt

if %errorlevel% equ 0 (
    echo All tests passed.
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

echo.--------------------------------------------------------------------------------
exit /b 0




:test
    echo.--------------------------------------------------------------------------------
    echo Test %testNum%: [%*]
    %timePrint% %*
    set /a testNum = testNum + 1
    goto :eof

:testCapture
    %timePrint% %2 %3 %4 %5 %6 %7 %8 %9 > %testOut%\test-output-%1.txt
    goto :eof

:testEqual
    echo.--------------------------------------------------------------------------------
    echo Test %testNum%: Output [%2 %3 %4 %5 %6 %7 %8 %9] equal to %1
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
    echo Test %testNum%: Error Test [%*]
    %timePrint% %* 1>nul 2>%TEMP%\timeprint-test-error-output.txt
    type %TEMP%\timeprint-test-error-output.txt
    del  %TEMP%\timeprint-test-error-output.txt
    set /a testNum = testNum + 1
    goto :eof
