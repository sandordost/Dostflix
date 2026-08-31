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
