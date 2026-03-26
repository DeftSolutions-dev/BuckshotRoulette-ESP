@echo off
where cl >/dev/null 2>&1 && goto :build
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >/dev/null 2>&1
if errorlevel 1 call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >/dev/null 2>&1

:build
cl /nologo /EHsc /O2 /std:c++17 /W3 /MT /Fe:BuckshotESP.exe src\main.cxx ^
   user32.lib gdi32.lib shell32.lib ole32.lib comdlg32.lib advapi32.lib
if exist BuckshotESP.exe (echo BUILD OK) else (echo BUILD FAILED)
del /q main.obj 2>/dev/null
