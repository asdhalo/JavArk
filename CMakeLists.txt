cmake_minimum_required(VERSION 3.16)
project(JavArk VERSION 3.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 启用安装目标的相对路径
if(WIN32)
    set(CMAKE_INSTALL_PREFIX "JavArk")
endif()

find_package(Qt6 COMPONENTS Core Gui Widgets Concurrent REQUIRED)

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

# 安装规则
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# 安装资源文件和配置文件(如果有)
# install(DIRECTORY resources DESTINATION .)

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
                    \"\${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}.exe\"
        )
    ")
endif()

# CPack配置
set(CPACK_PACKAGE_NAME "JavArk")
set(CPACK_PACKAGE_VENDOR "JavArk")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "视频管理库应用程序")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "JavArk")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_PACKAGE_EXECUTABLES "JavArk" "视频管理库")
    set(CPACK_NSIS_DISPLAY_NAME "视频管理库")
    set(CPACK_NSIS_PACKAGE_NAME "视频管理库")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    # 启用NSIS安装程序图标设置
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/app_icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/app_icon.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\${PROJECT_NAME}.exe")
    set(CPACK_NSIS_HELP_LINK "https://www.example.com/help")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://www.example.com/about")
    set(CPACK_NSIS_CONTACT "support@example.com")
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\视频管理库.lnk' '$INSTDIR\\\\bin\\\\${PROJECT_NAME}.exe'"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\视频管理库.lnk'"
    )
endif()

include(CPack) 