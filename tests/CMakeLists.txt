qt_add_executable(tst_app_startup smoke/tst_app_startup.cpp)
target_link_libraries(tst_app_startup PRIVATE Qt6::Core Qt6::Test)
dostflix_enable_warnings(tst_app_startup)
add_test(NAME app_startup COMMAND tst_app_startup)
