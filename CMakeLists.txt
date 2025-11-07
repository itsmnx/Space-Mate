cmake_minimum_required(VERSION 3.15)
project(Spacemate)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Enable AUTOMOC for Qt (generates meta-object code automatically)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt packages
find_package(Qt5 COMPONENTS 
    Widgets
    Concurrent
    REQUIRED
)

# Core sources
set(CORE_SOURCES
    core/backup_manager.cpp
    core/cleanup_manager.cpp
    core/disk_monitor.cpp
    core/file_analyzer.cpp
    core/utils.cpp
)

# CLI executable
add_executable(spacemate_cli main.cpp ${CORE_SOURCES})

# GUI executable
set(GUI_SOURCES
    gui/guimain.cpp
    gui/mainwindow.cpp
    ${CORE_SOURCES}   # reuse core sources
)

add_executable(SpacemateGUI ${GUI_SOURCES})
target_link_libraries(SpacemateGUI 
    Qt5::Widgets
    Qt5::Concurrent
)
