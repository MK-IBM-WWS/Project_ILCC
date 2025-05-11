cmake_minimum_required(VERSION 3.16)

project(ULSM VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    SysMonCore.cpp
    SysMonCore.hpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ULSM
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(ULSM ${PROJECT_SOURCES})
endif()

target_link_libraries(ULSM PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

find_library(SENSORS_LIB sensors)
if(SENSORS_LIB)
    message(STATUS "Found libsensors: ${SENSORS_LIB}")
    target_link_libraries(ULSM PRIVATE ${SENSORS_LIB})
else()
    message(WARNING "libsensors not found - temperature monitoring will be disabled")
endif()

include(GNUInstallDirs)
install(TARGETS ULSM
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ULSM)
endif()
