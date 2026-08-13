@echo off
setlocal

:: ==========================================
:: 1. Define Default Macros
:: ==========================================
set "HOST=kamino"
set "USER=dt100"
set "NCHAN=32"

:: ==========================================
:: 2. Bulletproof Argument Parser
:: ==========================================
:parse_args
if "%~1"=="" goto args_done

:: Assign argument to variable and strip surrounding quotes
set "arg=%~1"

if /i "%arg%"=="--help" goto show_help

:: Clear variables before parsing
set "key=%arg%"
set "val="

:: Safely split on equals sign (handles PowerShell passing "--key=value" as one string)
for /f "tokens=1* delims==" %%A in ("%arg%") do (
    set "key=%%A"
    set "val=%%B"
)

if defined val (
    :: Values were passed like --host=scarp (PowerShell style)
    if /i "%key%"=="--host" set "HOST=%val%"
    if /i "%key%"=="--user" set "USER=%val%"
    if /i "%key%"=="--chans" set "NCHAN=%val%"
) else (
    :: Values were passed like --host scarp (CMD style)
    if /i "%key%"=="--host" (
        set "HOST=%~2"
        shift
    ) else if /i "%key%"=="--user" (
        set "USER=%~2"
        shift
    ) else if /i "%key%"=="--chans" (
        set "NCHAN=%~2"
        shift
    ) else (
        :: Catch-all: treat unknown strings as the HOST (e.g. just typing 'scarp')
        set "HOST=%key%"
    )
)
shift
goto parse_args

:show_help
echo run_ht_scope_ui: Start phoebus gui for ht scope
echo.
echo Usage:
echo     run_ht_scope_ui.bat --host=kamino --user=dt100
echo.
echo Args:
echo     --host:      IOC host
echo     --user:      IOC user
echo     --chans:     Total chans per uut
exit /b 1

:args_done

:: ==========================================
:: 3. Define Paths
:: ==========================================
set "SCRIPT_DIR=%~dp0"

:: Safely get parent directory to prevent "Drive not found" errors
pushd "%SCRIPT_DIR%.."
set "PARENT_DIR=%CD%"
popd

set "JAVA_EXE=C:\Program Files\Eclipse Adoptium\jdk-17.0.14.7-hotspot\bin\java.exe"
set "PHOEBUS_JAR=C:\Users\craig\ACQ400CSSP\product-5.0.4\product-5.0.4.jar"
set "LAUNCHER=%PARENT_DIR%\CSS\ht_scope_launcher.bob"
set "SETTINGS=%SCRIPT_DIR%settings.ini"

:: ==========================================
:: 4. Build Macro Query and Resource URL
:: ==========================================
:: The quotes protect the '&' characters during assignment
set "QUERY=HOST=%HOST%&USER=%USER%&NCHAN=%NCHAN%"

:: Convert Windows backslashes (\) to forward slashes (/)
set "LAUNCHER_URI=%LAUNCHER:\=/%"

:: Java on Windows requires 'file:///' (3 slashes) for absolute C:/ paths
set "RESOURCE=file:///%LAUNCHER_URI%?%QUERY%"

:: ==========================================
:: 5. Clean Environment & Execute
:: ==========================================
if exist "%USERPROFILE%\.phoebus\memento" (
    del /q "%USERPROFILE%\.phoebus\memento"
)

echo CMD: "%JAVA_EXE%" -Dfile.encoding=UTF-8 -jar "%PHOEBUS_JAR%" -nosplash -settings "%SETTINGS%" -resource "%RESOURCE%" -layout null

:: Execute
"%JAVA_EXE%" -Dfile.encoding=UTF-8 -jar "%PHOEBUS_JAR%" -nosplash -settings "%SETTINGS%" -resource "%RESOURCE%" -layout null