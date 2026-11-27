@echo off
rem usage: build.bat <file.cpp> [extra cl flags]
call "%~dp0env.bat"
if not exist "%~dp0build" mkdir "%~dp0build"
set SRC=%~1
rem SoundArc.dll was built unoptimised with edit-and-continue debug info (/Od /ZI), unlike APP.dll and Packs.dll
cl /nologo /c /Od /ZI /Gy /GX /D WIN32 /D NDEBUG /D _WINDOWS /Fo"%~dp0build\\" /Fd"%~dp0build\vc70.pdb" %GURUMIN_CFLAGS% %2 %3 %4 %5 "%~dp0src\%SRC%"
if errorlevel 1 exit /b 1
for %%F in ("%SRC%") do "C:\Users\Michael\AppData\Local\Programs\Python\Python313\python.exe" "%~dp0tools\compare_obj.py" "%~dp0build\%%~nF.obj" %6



