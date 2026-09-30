@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 exit /b 1
cd /d "%~dp0"
if not exist build mkdir build
if not exist offsets mkdir offsets

set "CMAKE_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "%CMAKE_EXE%" exit /b 1
if not exist build\freetype\CMakeCache.txt (
  "%CMAKE_EXE%" -S vendor\freetype-2.14.3 -B build\freetype -G "Visual Studio 17 2022" -A x64 ^
    -D BUILD_SHARED_LIBS=OFF -D CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -D CMAKE_C_FLAGS=/utf-8 ^
    -D FT_DISABLE_ZLIB=ON -D FT_DISABLE_BZIP2=ON -D FT_DISABLE_PNG=ON ^
    -D FT_DISABLE_HARFBUZZ=ON -D FT_DISABLE_BROTLI=ON
  if errorlevel 1 exit /b 1
)
"%CMAKE_EXE%" --build build\freetype --config Release --target freetype --parallel 8
if errorlevel 1 exit /b 1

set INC=/I"src" /I"vendor/imgui" /I"vendor/imgui/backends" /I"vendor/minhook/include" /I"vendor/freetype-2.14.3/include"
set SRC=src\dllmain.cpp src\il2cpp_api.cpp src\game_data.cpp src\dx11_hook.cpp ^
 src\render\menu_ui.cpp src\render\menu_state.cpp src\render\menu.cpp src\render\ui_widgets.cpp src\render\ui_theme.cpp src\render\ui_lang.cpp ^
 src\config\config.cpp src\config\persist.cpp src\config\hotkeys.cpp src\app\app.cpp ^
 vendor\imgui\imgui.cpp vendor\imgui\imgui_draw.cpp vendor\imgui\imgui_tables.cpp vendor\imgui\imgui_widgets.cpp ^
 vendor\imgui\misc\freetype\imgui_freetype.cpp ^
 vendor\imgui\backends\imgui_impl_dx11.cpp vendor\imgui\backends\imgui_impl_win32.cpp ^
 vendor\minhook\src\hook.c vendor\minhook\src\buffer.c vendor\minhook\src\trampoline.c vendor\minhook\src\hde\hde64.c

cl /nologo /std:c++17 /O2 /MT /W3 /EHsc /utf-8 /D_WIN32_WINNT=0x0A00 /D_CRT_SECURE_NO_WARNINGS /DIMGUI_DEFINE_MATH_OPERATORS ^
  %INC% %SRC% ^
  /Fe:build\PVZOverlay.dll /Fo"build\\" /Fd"build\\" ^
  /link /DLL /OPT:REF /OPT:ICF user32.lib d3d11.lib dxgi.lib build\freetype\Release\freetype.lib
if errorlevel 1 (
  echo PVZOverlay.dll locked by running game - linking PVZOverlay_pending.dll instead
  cl /nologo /std:c++17 /O2 /MT /W3 /EHsc /utf-8 /D_WIN32_WINNT=0x0A00 /D_CRT_SECURE_NO_WARNINGS /DIMGUI_DEFINE_MATH_OPERATORS ^
    %INC% %SRC% ^
    /Fe:build\PVZOverlay_pending.dll /Fo"build\\" /Fd"build\\" ^
    /link /DLL /OPT:REF /OPT:ICF user32.lib d3d11.lib dxgi.lib build\freetype\Release\freetype.lib
  if errorlevel 1 (
    echo dll_exit=2
    exit /b 1
  )
  start "" /b cmd /c "%~dp0sync_dll.bat"
)
echo dll_exit=0

cl /nologo /std:c++17 /O2 /MT /W3 /EHsc /utf-8 /D_CRT_SECURE_NO_WARNINGS ^
  src\injector\injector.cpp ^
  /Fe:build\injector.exe /Fo"build\inj_" ^
  /link /OPT:REF /OPT:ICF advapi32.lib
echo inj_exit=%errorlevel%
if errorlevel 1 exit /b 1

endlocal
