cmake_minimum_required(VERSION 3.21)
project(qt_monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Multimedia MultimediaWidgets)

qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/boardlink.cpp
    src/videopanel.cpp
    src/rtspstreamer.cpp
    src/distanceplot.cpp
    src/logpanel.cpp
    src/heatmap.cpp
)

set(HEADERS
    src/mainwindow.h
    src/boardlink.h
    src/videopanel.h
    src/rtspstreamer.h
    src/distanceplot.h
    src/logpanel.h
    src/heatmap.h
    src/algstate.h
)

set(FORMS
    src/mainwindow.ui
)

set(RESOURCES
    resources/resources.qrc
)

if(WIN32)
    set(WIN32_RC resources/qt_monitor.rc)
endif()

# FFmpeg: 链接 Qt 自带的 9.0.1 dll (与 Qt 同 MinGW 工具链构建, 运行时已由 windeployqt 部署)
# 头文件取自官方 ffmpeg-9.0.1 源码包 (third_party/ffmpeg/include), ABI 与 dll 完全对齐
get_target_property(QT6_CORE_DLL Qt6::Core IMPORTED_LOCATION)
if(NOT QT6_CORE_DLL)
    set(QT6_CORE_DLL "${CMAKE_PREFIX_PATH}/bin/Qt6Core.dll")
endif()
get_filename_component(FFMPEG_BIN "${QT6_CORE_DLL}" DIRECTORY)
set(FFMPEG_LIBS
    ${FFMPEG_BIN}/avutil-61.dll
    ${FFMPEG_BIN}/avcodec-63.dll
    ${FFMPEG_BIN}/avformat-63.dll
    ${FFMPEG_BIN}/swscale-10.dll
)

qt_add_executable(qt_monitor WIN32 ${SOURCES} ${HEADERS} ${FORMS} ${RESOURCES})
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${WIN32_RC}")
    target_sources(qt_monitor PRIVATE "${WIN32_RC}")
endif()

target_link_libraries(qt_monitor PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ${FFMPEG_LIBS}
)

target_include_directories(qt_monitor PRIVATE
    src
    third_party/ffmpeg/include
)

if(WIN32)
    set_target_properties(qt_monitor PROPERTIES
        OUTPUT_NAME "qt_monitor"
    )
endif()
