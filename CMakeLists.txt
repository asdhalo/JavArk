cmake_minimum_required(VERSION 3.16)
project(JavArk VERSION 5.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 启用安装目标的相对路径 - 便携版
if(WIN32)
    set(CMAKE_INSTALL_PREFIX "JavArk-Portable")
endif()

# 设置Qt6的路径
set(CMAKE_PREFIX_PATH "D:/Qt6/6.8.3/msvc2022_64")

find_package(Qt6 COMPONENTS Core Gui Widgets Concurrent REQUIRED)

# 检查FFmpeg
message(STATUS "Checking for FFmpeg...")
find_program(FFMPEG_EXECUTABLE ffmpeg)
find_program(FFPROBE_EXECUTABLE ffprobe)

if(FFMPEG_EXECUTABLE AND FFPROBE_EXECUTABLE)
    message(STATUS "FFmpeg found: ${FFMPEG_EXECUTABLE}")
    message(STATUS "FFprobe found: ${FFPROBE_EXECUTABLE}")
else()
    message(WARNING "FFmpeg not found. Video frame extraction feature will not work properly.")
    message(WARNING "Please install FFmpeg and make sure it's in your PATH.")
endif()


set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/videoitem.cpp
    src/videolibrary.cpp
)

set(HEADERS
    include/mainwindow.h
    include/videoitem.h
    include/videolibrary.h
)

set(RESOURCES
    resources/resources.qrc
)

# 改为WIN32应用程序类型，隐藏控制台窗口
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
)

# 为Windows设置应用程序图标
if(WIN32)
    # 启用图标设置
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/app_icon.rc")
    if(EXISTS "${APP_ICON_RESOURCE}")
        target_sources(${PROJECT_NAME} PRIVATE "${APP_ICON_RESOURCE}")
    endif()
endif()

# 安装规则 - 便携版直接安装到根目录
install(TARGETS ${PROJECT_NAME} DESTINATION .)

# 安装资源文件和配置文件
# 创建日志目录
install(DIRECTORY DESTINATION logs)

# 安装FFmpeg文件
if(FFMPEG_EXECUTABLE AND FFPROBE_EXECUTABLE)
    get_filename_component(FFMPEG_DIR "${FFMPEG_EXECUTABLE}" DIRECTORY)
    install(FILES
        "${FFMPEG_EXECUTABLE}"
        "${FFPROBE_EXECUTABLE}"
        DESTINATION .
    )

    # 安装FFmpeg依赖的DLL文件
    file(GLOB FFMPEG_DLLS "${FFMPEG_DIR}/*.dll")
    install(FILES ${FFMPEG_DLLS} DESTINATION .)
endif()

# 使用windeployqt工具部署Qt依赖
if(WIN32)
    # 查找windeployqt.exe的位置
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    # 安装后运行windeployqt
    install(CODE "
        message(STATUS \"Running windeployqt...\")
        execute_process(
            COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                    --no-translations
                    --no-system-d3d-compiler
                    --no-opengl-sw
                    \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.exe\"
        )
    ")
endif()

# CPack配置 - 只使用ZIP包便于U盘便携版
set(CPACK_PACKAGE_NAME "JavArk-Portable")
set(CPACK_PACKAGE_VENDOR "JavArk")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "视频管理库应用程序 - 便携版")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "JavArk-Portable-${PROJECT_VERSION}")

if(WIN32)
    # 只使用ZIP包，不使用NSIS安装程序
    set(CPACK_GENERATOR "ZIP")
endif()

include(CPack)
