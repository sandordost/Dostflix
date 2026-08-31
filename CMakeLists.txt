cmake_minimum_required(VERSION 3.28)
project(Dostflix VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core DBus Gui Network Qml Quick QuickControls2 QuickTest Sql Test)
qt_standard_project_setup(REQUIRES 6.8)

include(cmake/DostflixWarnings.cmake)

qt_add_executable(dostflix
    src/main.cpp
    src/app/AppPaths.cpp
    src/app/AppSettings.cpp
    src/library/LibraryDatabase.cpp
    src/movies/Movie.cpp
    src/movies/MovieListModel.cpp
    src/ui/AppController.cpp
    src/vpn/NetworkManagerBackend.cpp
    src/vpn/VpnManager.cpp
    src/vpn/VpnProfileModel.cpp
)
target_include_directories(dostflix PRIVATE src)

qt_add_library(dostflix_ui STATIC)
set_source_files_properties(qml/theme/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(dostflix_ui
    URI Dostflix
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Dostflix
    QML_FILES
        qml/Main.qml
        qml/components/AppHeader.qml
        qml/components/MovieCard.qml
        qml/components/MovieGrid.qml
        qml/components/NowWatchingCard.qml
        qml/components/SidePanel.qml
        qml/pages/DiscoverPage.qml
        qml/pages/DownloadsPage.qml
        qml/pages/LibraryPage.qml
        qml/pages/SettingsPage.qml
        qml/theme/Theme.qml
    RESOURCES
        assets/backgrounds/dust-background.jpg
        assets/icons/dostflix.svg
)
target_link_libraries(dostflix_ui PRIVATE Qt6::Quick Qt6::QuickControls2)
target_link_libraries(dostflix PRIVATE
    Qt6::Core Qt6::DBus Qt6::Gui Qt6::Quick Qt6::QuickControls2 Qt6::Sql dostflix_uiplugin
)
dostflix_enable_warnings(dostflix)

qt_add_executable(dostflix-network-helper
    src/network-helper/main.cpp
    src/network/NetworkGuardRules.cpp
)
target_include_directories(dostflix-network-helper PRIVATE src)
target_link_libraries(dostflix-network-helper PRIVATE Qt6::Core Qt6::Network)
dostflix_enable_warnings(dostflix-network-helper)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(TARGETS dostflix RUNTIME DESTINATION lib/dostflix)
install(TARGETS dostflix-network-helper RUNTIME DESTINATION lib/dostflix)
install(PROGRAMS packaging/dostflix-launcher DESTINATION bin RENAME dostflix)
install(FILES packaging/io.github.sandordost.Dostflix.network-guard.policy
        DESTINATION share/polkit-1/actions)
install(FILES packaging/io.github.sandordost.Dostflix.desktop
        DESTINATION share/applications)
install(FILES assets/icons/dostflix.svg
        DESTINATION share/icons/hicolor/scalable/apps
        RENAME io.github.sandordost.Dostflix.svg)
