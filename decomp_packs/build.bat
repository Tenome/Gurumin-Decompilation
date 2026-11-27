@echo off
rem usage: build.bat <file.cpp> [extra cl flags]
call "%~dp0env.bat"
if not exist "%~dp0build" mkdir "%~dp0build"
set SRC=%~1
cl /nologo /c /O2 /Ob1 /Op /Gy /GS /GX /D WIN32 /D NDEBUG /D _WINDOWS /Fo"%~dp0build\\" %GURUMIN_CFLAGS% %2 %3 %4 %5 "%~dp0src\%SRC%"
if errorlevel 1 exit /b 1
for %%F in ("%SRC%") do "C:\Users\Michael\AppData\Local\Programs\Python\Python313\python.exe" "%~dp0tools\compare_obj.py" "%~dp0build\%%~nF.obj" %6



