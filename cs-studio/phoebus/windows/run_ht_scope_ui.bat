@echo off
setlocal

:: ==========================================
:: 1. Define Default Macros
:: ==========================================
set "HOST=kamino"
set "USER=dt100"
set "NCHAN=32"
set "JAVA_EXE="
set "PHOEBUS_JAR="

:: Establish relative directories
set "SCRIPT_DIR=%~dp0"
pushd "%SCRIPT_DIR%.."
set "PARENT_DIR=%CD%"
popd

:: ==========================================
:: 2. Bulletproof Argument Parser
:: ==========================================
:parse_args
if "%~1"=="" goto args_done
set "arg=%~1"
if /i "%arg%"=="--help" goto show_help

set "key=%arg%"
set "val="

:: Safely split on equals sign
for /f "tokens=1* delims==" %%A in ("%arg%") do (
    set "key=%%A"
    set "val=%%B"
)

:: If value exists, skip to the value handler
if defined val goto args_has_val

:: --- We don't have an equals sign (e.g., --host torsa) ---
set "next_arg=%~2"
if /i "%key%"=="--host" set "HOST=%next_arg%" & shift & shift & goto parse_args
if /i "%key%"=="--user" set "USER=%next_arg%" & shift & shift & goto parse_args
if /i "%key%"=="--chans" set "NCHAN=%next_arg%" & shift & shift & goto parse_args
if /i "%key%"=="--java" set "JAVA_EXE=%next_arg%" & shift & shift & goto parse_args
if /i "%key%"=="--phoebus" set "PHOEBUS_JAR=%next_arg%" & shift & shift & goto parse_args

:: Catch-all: treat unknown strings as the HOST
set "HOST=%key%"
shift
goto parse_args

:args_has_val
:: --- We have a value (e.g., --host=torsa) ---
:: Remove quotes from val safely to prevent double-quoting crashes
set "val=%val:"=%"
if /i "%key%"=="--host" set "HOST=%val%"
if /i "%key%"=="--user" set "USER=%val%"
if /i "%key%"=="--chans" set "NCHAN=%val%"
if /i "%key%"=="--java" set "JAVA_EXE=%val%"
if /i "%key%"=="--phoebus" set "PHOEBUS_JAR=%val%"
shift
goto parse_args

:show_help
echo run_ht_scope_ui: Start phoebus gui for ht scope
echo.
echo Usage: run_ht_scope_ui.bat [options]
echo.
echo Options:
echo     --host=NAME      IOC host (default: kamino)
echo     --user=NAME      IOC user (default: dt100)
echo     --chans=NUM      Total chans per uut (default: 32)
echo     --java=PATH      Custom path to java.exe
echo     --phoebus=PATH   Custom path to phoebus jar
exit /b 1

:args_done

:: ==========================================
:: 3. Resolve Executables and Paths
:: ==========================================

:: --- Resolve Java ---
if defined JAVA_EXE goto java_resolved

where java >nul 2>&1
if %errorlevel% equ 0 set "JAVA_EXE=java" & goto java_resolved

if defined JAVA_HOME set "JAVA_EXE=%JAVA_HOME%\bin\java.exe" & goto java_resolved

echo ERROR: Java not found in PATH and JAVA_HOME is not set.
echo Please install Java or specify path via --java="C:\path\to\java.exe"
exit /b 1

:java_resolved


:: --- Resolve Phoebus JAR ---
if defined PHOEBUS_JAR goto jar_resolved

:: Native recursive search, much safer than 'dir /s'
pushd "%PARENT_DIR%"
for /r %%I in (product-*.jar) do set "PHOEBUS_JAR=%%I"
popd

if defined PHOEBUS_JAR goto jar_resolved

echo ERROR: Phoebus JAR (product-*.jar) not found inside %PARENT_DIR%.
echo Please ensure it is downloaded.
exit /b 1

:jar_resolved

:: Ensure quotes are stripped for the exist check
set "PHOEBUS_JAR=%PHOEBUS_JAR:"=%"
if exist "%PHOEBUS_JAR%" goto jar_exists
echo ERROR: Phoebus JAR not found at "%PHOEBUS_JAR%"
exit /b 1

:jar_exists

:: --- Other Paths ---
set "LAUNCHER=%PARENT_DIR%\CSS\ht_scope_launcher.bob"
set "SETTINGS=%SCRIPT_DIR%settings.ini"

if exist "%LAUNCHER%" goto launcher_exists
echo WARNING: Launcher file not found at "%LAUNCHER%"
:launcher_exists

:: ==========================================
:: 4. Build Macro Query and Resource URL
:: ==========================================
set "QUERY=HOST=%HOST%&USER=%USER%&NCHAN=%NCHAN%"
set "LAUNCHER_URI=%LAUNCHER:\=/%"
set "RESOURCE=file:///%LAUNCHER_URI%?%QUERY%"

:: ==========================================
:: 5. Clean Environment & Execute
:: ==========================================
set "MEMENTO_PATH=%USERPROFILE%\.phoebus\memento"
if exist "%MEMENTO_PATH%" del /q "%MEMENTO_PATH%"

echo.
echo ========================================
echo Starting Phoebus GUI...
echo Target:  %HOST% (%USER%, %NCHAN% chans)
echo Java:    %JAVA_EXE%
echo Phoebus: %PHOEBUS_JAR%
echo ========================================
echo.

"%JAVA_EXE%" -Dfile.encoding=UTF-8 -jar "%PHOEBUS_JAR%" -nosplash -settings "%SETTINGS%" -resource "%RESOURCE%" -layout null