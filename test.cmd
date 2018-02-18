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

call :test -h
call :test -H
call :test /h
call :test /H
call :test -?
call :test "/?"
call :test --help
call :test --HELP

call :errTest --
call :errTest -
call :errTest --bogusSwitch
call :errTest -m
call :errTest --modTime
call :errTest --modTime someBogusFile
call :errTest --modTime file1 --modTime file2 --modTime bogusThirdOption
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
