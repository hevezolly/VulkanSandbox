@echo off
set build_path="%1\\build"
cmake -S %1 -B %build_path%
cmake --build %build_path% --config %2

IF not %ERRORLEVEL% EQU 0 exit /b %ERRORLEVEL%

IF "%3"=="-e" %build_path%\\%2\\VulkanSandbox.exe