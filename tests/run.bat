@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
if not exist build mkdir build
cl /nologo /std:c++17 /EHsc /W4 /Fe:build\projection_math_test.exe /Fo:build\projection_math_test.obj tests\projection_math_test.cpp
if errorlevel 1 exit /b 1
build\projection_math_test.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /utf-8 /D_CRT_SECURE_NO_WARNINGS /Isrc /Fe:build\esp_color_test.exe /Fo"build\\" tests\esp_color_test.cpp src\render\menu_state.cpp src\config\config.cpp
if errorlevel 1 exit /b 1
build\esp_color_test.exe
if errorlevel 1 exit /b 1
if not exist build\freetype\Release\freetype.lib call build.bat
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W3 /utf-8 /D_CRT_SECURE_NO_WARNINGS /DIMGUI_DEFINE_MATH_OPERATORS ^
  /Ivendor\imgui /Ivendor\freetype-2.14.3\include ^
  /Fe:build\font_atlas_test.exe /Fo"build\\" ^
  tests\font_atlas_test.cpp vendor\imgui\imgui.cpp vendor\imgui\imgui_draw.cpp ^
  vendor\imgui\imgui_tables.cpp vendor\imgui\imgui_widgets.cpp ^
  vendor\imgui\misc\freetype\imgui_freetype.cpp ^
  /link build\freetype\Release\freetype.lib
if errorlevel 1 exit /b 1
build\font_atlas_test.exe
exit /b %errorlevel%
