cmake_minimum_required(VERSION 3.28)
project(Dostflix VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Sql Test)
qt_standard_project_setup(REQUIRES 6.8)

include(cmake/DostflixWarnings.cmake)

qt_add_executable(dostflix src/main.cpp)
qt_add_qml_module(dostflix
    URI Dostflix
    VERSION 1.0
    QML_FILES qml/Main.qml
)
target_link_libraries(dostflix PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2 Qt6::Sql
)
dostflix_enable_warnings(dostflix)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(TARGETS dostflix RUNTIME DESTINATION bin)
