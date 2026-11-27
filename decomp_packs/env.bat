@echo off
rem Default: VS .NET 2002 (cl 13.00.9466), the compiler APP.dll and the helper DLLs were built with.
rem Set GURUMIN_VC to another Vc7 folder (e.g. VS .NET 2003) to override.
set "VC=E:\Visual StudioNET 2002\Vc7"
if defined GURUMIN_VC set "VC=%GURUMIN_VC%"
set "PATH=%VC%\bin;%VC%\..\Common7\IDE;%PATH%"
set "INCLUDE=%~dp0include;%VC%\include;%VC%\PlatformSDK\include"
set "LIB=%VC%\lib;%VC%\PlatformSDK\lib"
