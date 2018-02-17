@echo off & setlocal
: ==================================================================================================
: Acceptance testing for `timeprint`. Note that to properly pass % characters, they must be escaped
: (sometimes twice).
:
: NOTE: This test requires the `diff` tool to be on your path.
: ==================================================================================================

if "%1" equ "--run" goto :runTests

call %0 --run > tests-output.txt
diff tests-accepted.txt tests-output.txt

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

call :test A b c d e Hello world f g h i j
call :test "A b c d e Hello world f g h i j"
call :test "A\nB\nC"
call :test "A\tB\tC"
call :test "A%%%%nB%%%%nC"
call :test "A%%%%tB%%%%tC"
call :test Percent sign = %%%%%%%%

echo.--------------------------------------------------------------------------------
exit /b 0



:test
    echo.--------------------------------------------------------------------------------
    echo [%*]
    x64\Release\timeprint.exe %*
    goto :eof
