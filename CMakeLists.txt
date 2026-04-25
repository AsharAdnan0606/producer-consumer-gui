cmake_minimum_required(VERSION 3.16)
project(HospitalQueue VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Works with both Qt5 and Qt6
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(HospitalQueue
    main.cpp
    mainwindow.cpp
    mainwindow.h
    hospitalengine.cpp
    hospitalengine.h
)

target_link_libraries(HospitalQueue
    Qt${QT_VERSION_MAJOR}::Widgets
    pthread
)

if(WIN32)
    set_target_properties(HospitalQueue PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
