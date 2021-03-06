@echo off
setlocal EnableDelayedExpansion

set "SOURCE_DIR=%~dp0"

if "%1" == "x64" (
  set ARCHITECTURE=x64
  set GENERATOR=Visual Studio 11 2012 Win64
) else if "%1" == "x86" (
  set ARCHITECTURE=x86
  set GENERATOR=Visual Studio 11 2012
) else (
  echo Usage: build x86^|x64 [zip]
  exit /b 1
)

cmake -G "%GENERATOR%" -T v110_xp "%SOURCE_DIR%"
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build . --config Release --target ALL_BUILD
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build . --config Release --target check
if %errorlevel% neq 0 exit /b %errorlevel%

copy Release\BFArchive.exe .

if "%2" == "zip" (
  7z a -tzip bfarchive-win-%ARCHITECTURE%.zip BFArchive.exe
)
