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

qt_add_executable(tst_library_manager
    library/tst_library_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/app/AppSettings.cpp
    ${PROJECT_SOURCE_DIR}/src/library/LibraryDatabase.cpp
    ${PROJECT_SOURCE_DIR}/src/library/LibraryManager.cpp
    ${PROJECT_SOURCE_DIR}/src/library/LocalLibraryModel.cpp
)
target_include_directories(tst_library_manager PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_library_manager PRIVATE Qt6::Core Qt6::Sql Qt6::Test)
dostflix_enable_warnings(tst_library_manager)
add_test(NAME library_manager COMMAND tst_library_manager)

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
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Basic;QSG_RHI_BACKEND=opengl"
)

qt_add_executable(tst_app_controller
    ui/tst_app_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/ui/AppController.cpp
)
target_include_directories(tst_app_controller PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_app_controller PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_app_controller)
add_test(NAME app_controller COMMAND tst_app_controller)

qt_add_executable(tst_mpv_player player/tst_mpv_player.cpp)
target_include_directories(tst_mpv_player PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_mpv_player PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Test PkgConfig::MPV dostflix_ui
)
dostflix_enable_warnings(tst_mpv_player)
add_test(NAME mpv_player COMMAND tst_mpv_player)
set_tests_properties(mpv_player PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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

qt_add_executable(tst_release_resolver
    providers/tst_release_resolver.cpp
    ${PROJECT_SOURCE_DIR}/src/providers/ReleaseResolver.cpp
)
target_include_directories(tst_release_resolver PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_release_resolver PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_release_resolver)
add_test(NAME release_resolver COMMAND tst_release_resolver)

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

qt_add_executable(tst_open_subtitles_manager
    subtitles/tst_open_subtitles_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/subtitles/OpenSubtitlesManager.cpp
)
target_include_directories(tst_open_subtitles_manager PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_open_subtitles_manager PRIVATE Qt6::Core Qt6::Network Qt6::Test)
dostflix_enable_warnings(tst_open_subtitles_manager)
add_test(NAME open_subtitles_manager
    COMMAND /usr/bin/unshare --user --map-root-user --net /usr/bin/sh -c
            "/usr/bin/ip link set lo up; exec '$<TARGET_FILE:tst_open_subtitles_manager>'"
)

qt_add_executable(tst_secret_store
    providers/tst_secret_store.cpp
    ${PROJECT_SOURCE_DIR}/src/providers/SecretStore.cpp
)
target_include_directories(tst_secret_store PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_secret_store PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_secret_store)
add_test(NAME secret_store COMMAND tst_secret_store)

qt_add_executable(tst_buffer_controller
    streaming/tst_buffer_controller.cpp
    ${PROJECT_SOURCE_DIR}/src/streaming/BufferController.cpp
)
target_include_directories(tst_buffer_controller PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_buffer_controller PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_buffer_controller)
add_test(NAME buffer_controller COMMAND tst_buffer_controller)

qt_add_executable(tst_torrent_file_model
    streaming/tst_torrent_file_model.cpp
    ${PROJECT_SOURCE_DIR}/src/streaming/TorrentFileModel.cpp
)
target_include_directories(tst_torrent_file_model PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_torrent_file_model PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_torrent_file_model)
add_test(NAME torrent_file_model COMMAND tst_torrent_file_model)

qt_add_executable(tst_torrserver_manager
    streaming/tst_torrserver_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/streaming/BufferController.cpp
    ${PROJECT_SOURCE_DIR}/src/streaming/TorrServerManager.cpp
    ${PROJECT_SOURCE_DIR}/src/streaming/TorrentFileModel.cpp
)
target_include_directories(tst_torrserver_manager PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_torrserver_manager PRIVATE Qt6::Core Qt6::Network Qt6::Test)
dostflix_enable_warnings(tst_torrserver_manager)
add_test(NAME torrserver_manager
    COMMAND /usr/bin/unshare --user --map-root-user --net /usr/bin/sh -c
            "/usr/bin/ip link set lo up; exec '$<TARGET_FILE:tst_torrserver_manager>'"
)

qt_add_executable(tst_stream_server
    streaming/tst_stream_server.cpp
    ${PROJECT_SOURCE_DIR}/src/streaming/StreamServer.cpp
)
target_include_directories(tst_stream_server PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_stream_server PRIVATE Qt6::Core Qt6::Network Qt6::Test)
dostflix_enable_warnings(tst_stream_server)
add_test(NAME stream_server COMMAND tst_stream_server)
