@echo off & setlocal
: ==================================================================================================
: Acceptance testing for `timeprint`. Note that to properly pass % characters, they must be escaped
: (sometimes twice).
:
: NOTE: This test requires the `diff` tool to be on your path.
: ==================================================================================================

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

call %0 --run %timePrint% > tests-output.txt
diff -u3 tests-accepted.txt tests-output.txt

if %errorlevel% equ 0 (
    echo All tests passed.
    goto :eof
)

echo.
set /p response="Differences found. Accept new results? "
if /i "%response%" neq "y"  if /i "%response%" neq "yes"  exit /b 1

copy tests-output.txt tests-accepted.txt
exit /b 0



: ==================================================================================================
:runTests

set timePrint=%2

echo Acceptance Tests for `timeprint`
echo ================================================================================
echo.

rem call :test -h
rem call :test -H
rem call :test /h
rem call :test /H
rem call :test -?
rem call :test "/?"
rem call :test --HELP
call :test --help

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
    echo [%*]
    %timePrint% %*
    goto :eof

:errTest
    echo.--------------------------------------------------------------------------------
    echo Error Test [%*]
    %timePrint% %* 1>nul 2>%TEMP%\timeprint-test-error-output.txt
    type %TEMP%\timeprint-test-error-output.txt
    del  %TEMP%\timeprint-test-error-output.txt
    goto :eof
