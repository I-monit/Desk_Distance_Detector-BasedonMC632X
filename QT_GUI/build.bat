@echo off
setlocal
set QT_ROOT=D:\Apps\Qt_GUI\QT_App\6.12.0\mingw_64
set MINGW=D:\Apps\Qt_GUI\QT_App\Tools\mingw1310_64\bin
set CMAKE=D:\Apps\Qt_GUI\QT_App\Tools\CMake_64\bin
set NINJA=D:\Apps\Qt_GUI\QT_App\Tools\Ninja

set PATH=%MINGW%;%CMAKE%;%NINJA%;%QT_ROOT%\bin;%PATH%

cmake -S "%~dp0" -B "%~dp0build" -G Ninja ^
    -DCMAKE_PREFIX_PATH=%QT_ROOT% ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo CMake configure failed.
    exit /b 1
)

cmake --build "%~dp0build"

if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

"%QT_ROOT%\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-opengl-sw "%~dp0build\qt_monitor.exe"

echo.
echo Build and deploy OK.
