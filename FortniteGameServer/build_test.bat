@echo off
REM Build validation script for Fortnite Game Server DLL

echo ==========================================
echo   Fortnite Game Server - Build Test
echo ==========================================
echo.

REM Check if Visual Studio is available
where msbuild >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: MSBuild not found in PATH
    echo Please run this from a Visual Studio Developer Command Prompt
    pause
    exit /b 1
)

echo [1/4] Checking project files...
if not exist "FortniteGameServer.sln" (
    echo ERROR: Solution file not found
    pause
    exit /b 1
)

if not exist "FortniteGameServer.vcxproj" (
    echo ERROR: Project file not found
    pause
    exit /b 1
)

echo [2/4] Building Debug configuration...
msbuild FortniteGameServer.sln /p:Configuration=Debug /p:Platform=x64 /v:minimal
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Debug build failed
    pause
    exit /b 1
)

echo [3/4] Building Release configuration...
msbuild FortniteGameServer.sln /p:Configuration=Release /p:Platform=x64 /v:minimal
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Release build failed
    pause
    exit /b 1
)

echo [4/4] Validating output...
if exist "bin\Debug\FortniteGameServer.dll" (
    echo ✓ Debug DLL built successfully
) else (
    echo ✗ Debug DLL not found
)

if exist "bin\Release\FortniteGameServer.dll" (
    echo ✓ Release DLL built successfully
) else (
    echo ✗ Release DLL not found
)

echo.
echo ==========================================
echo   Build validation completed!
echo ==========================================
echo.
echo The DLL is ready for injection into Fortnite.
echo Once injected, it will automatically start
echo hosting a server for players to join.
echo.
pause