cmake_minimum_required(VERSION 3.16)
project(json-viewer-qt VERSION 2.0.7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()

# ── 共享库 libs/json-view（Pulse-qt 等可复用）──
add_library(jsonview STATIC
    lib/JsonTreeModel.cpp
    lib/JsonTreeModel.h
    lib/JsonTreeDelegate.cpp
    lib/JsonTreeDelegate.h
)
target_include_directories(jsonview PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(jsonview PUBLIC Qt6::Widgets)

# Windows 图标资源：仅在 Windows 嵌入 exe（.rc 编译进 PE 资源，任务栏/开始菜单读取）
set(WIN_ICON_RESOURCE "")
if(WIN32)
    set(WIN_ICON_RESOURCE resources/icon.rc)
endif()

# WIN32 标志：Windows 下编译为 GUI 子系统，避免启动弹出黑色控制台窗口（Linux 无影响）
qt_add_executable(json-viewer-qt WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    resources/resources.qrc
    ${WIN_ICON_RESOURCE}
)

target_link_libraries(json-viewer-qt PRIVATE jsonview Qt6::Widgets Qt6::Network)

# 版本号统一注入：开屏示例数据从 CMake VERSION 读取，避免与 tag 漂移
target_compile_definitions(json-viewer-qt PRIVATE APP_VERSION="${PROJECT_VERSION}")

# 安装规则（RPM 打包用）
include(GNUInstallDirs)
install(TARGETS json-viewer-qt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES resources/icon.png DESTINATION ${CMAKE_INSTALL_DATADIR}/json-viewer-qt)
install(FILES packaging/json-viewer-qt.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
