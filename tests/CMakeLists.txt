qt_add_executable(tst_app_startup smoke/tst_app_startup.cpp)
target_link_libraries(tst_app_startup PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_app_startup)
add_test(NAME app_startup COMMAND tst_app_startup)

qt_add_executable(tst_app_paths
    app/tst_app_paths.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AppPaths.cpp
)
target_include_directories(tst_app_paths PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_app_paths PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_app_paths)
add_test(NAME app_paths COMMAND tst_app_paths)

qt_add_executable(tst_app_settings
    app/tst_app_settings.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AppSettings.cpp
)
target_include_directories(tst_app_settings PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_app_settings PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_app_settings)
add_test(NAME app_settings COMMAND tst_app_settings)

qt_add_executable(tst_library_database
    library/tst_library_database.cpp
    ${PROJECT_SOURCE_DIR}/src/library/LibraryDatabase.cpp
)
target_include_directories(tst_library_database PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_library_database PRIVATE Qt6::Core Qt6::Sql Qt6::Test)
dostflix_enable_warnings(tst_library_database)
add_test(NAME library_database COMMAND tst_library_database)

qt_add_executable(tst_movie_list_model
    movies/tst_movie_list_model.cpp
    ${PROJECT_SOURCE_DIR}/src/movies/Movie.cpp
    ${PROJECT_SOURCE_DIR}/src/movies/MovieListModel.cpp
)
target_include_directories(tst_movie_list_model PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_movie_list_model PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_movie_list_model)
add_test(NAME movie_list_model COMMAND tst_movie_list_model)

qt_add_executable(tst_qml_components qml/tst_qml.cpp)
target_link_libraries(tst_qml_components PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickTest dostflix_uiplugin
)
dostflix_enable_warnings(tst_qml_components)
add_test(NAME qml_component_tests COMMAND tst_qml_components
    -input ${CMAKE_CURRENT_SOURCE_DIR}/qml
)
set_tests_properties(qml_component_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Basic"
)

qt_add_executable(tst_app_controller
    ui/tst_app_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/AppController.cpp
)
target_include_directories(tst_app_controller PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_app_controller PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_app_controller)
add_test(NAME app_controller COMMAND tst_app_controller)

qt_add_executable(tst_network_manager_backend
    vpn/tst_network_manager_backend.cpp
    ${PROJECT_SOURCE_DIR}/src/vpn/NetworkManagerBackend.cpp
)
target_include_directories(tst_network_manager_backend PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_network_manager_backend PRIVATE Qt6::Core Qt6::DBus Qt6::Network Qt6::Test)
dostflix_enable_warnings(tst_network_manager_backend)
add_test(NAME network_manager_backend COMMAND tst_network_manager_backend)

qt_add_executable(tst_vpn_manager
    vpn/tst_vpn_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AppSettings.cpp
    ${PROJECT_SOURCE_DIR}/src/vpn/VpnManager.cpp
    ${PROJECT_SOURCE_DIR}/src/vpn/VpnProfileModel.cpp
)
target_include_directories(tst_vpn_manager PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_vpn_manager PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_vpn_manager)
add_test(NAME vpn_manager COMMAND tst_vpn_manager)

qt_add_executable(tst_network_guard_rules
    network/tst_network_guard_rules.cpp
    ${PROJECT_SOURCE_DIR}/src/network/NetworkGuardRules.cpp
)
target_include_directories(tst_network_guard_rules PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_network_guard_rules PRIVATE Qt6::Core Qt6::Network Qt6::Test)
dostflix_enable_warnings(tst_network_guard_rules)
add_test(NAME network_guard_rules COMMAND tst_network_guard_rules)

qt_add_executable(tst_torznab_parser
    providers/tst_torznab_parser.cpp
    ${PROJECT_SOURCE_DIR}/src/providers/TorznabParser.cpp
)
target_include_directories(tst_torznab_parser PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_torznab_parser PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_torznab_parser)
add_test(NAME torznab_parser COMMAND tst_torznab_parser)

qt_add_executable(tst_provider_manager
    providers/tst_provider_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AppSettings.cpp
    ${PROJECT_SOURCE_DIR}/src/providers/ProviderListModel.cpp
    ${PROJECT_SOURCE_DIR}/src/providers/ProviderManager.cpp
)
target_include_directories(tst_provider_manager PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_provider_manager PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_provider_manager)
add_test(NAME provider_manager COMMAND tst_provider_manager)
