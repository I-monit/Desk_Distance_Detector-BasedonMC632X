@echo off
chcp 65001 >nul
setlocal

rem 增量编译, 保证启动最新构建 (首次配置请先跑 build.bat)
set QT_ROOT=D:\Apps\Qt_GUI\QT_App\6.12.0\mingw_64
set MINGW=D:\Apps\Qt_GUI\QT_App\Tools\mingw1310_64\bin
set CMAKE=D:\Apps\Qt_GUI\QT_App\Tools\CMake_64\bin
set NINJA=D:\Apps\Qt_GUI\QT_App\Tools\Ninja
set PATH=%MINGW%;%CMAKE%;%NINJA%;%QT_ROOT%\bin;%PATH%

set QT_FFMPEG_RTSP_TRANSPORT=tcp

cmake --build "%~dp0build"
if errorlevel 1 (
    echo.
    echo Build FAILED, not launching stale exe. Fix errors or run build.bat first.
    pause
    exit /b 1
)

cd /d "%~dp0build"
start "" "qt_monitor.exe"
