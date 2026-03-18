find_package(GTest CONFIG REQUIRED)

add_executable(w3terrain_test 
    main.cpp
    collector_test.cpp
    section_manager_test.cpp
    runtime_manager_test.cpp
    informator_test.cpp
    informator_ramp_test.cpp
)
target_link_libraries(w3terrain_test PRIVATE ${LIBNAME}_options)
target_link_libraries(w3terrain_test PRIVATE
    GTest::gtest_main
    GTest::gmock
    godot::cpp
    quadtree::quadtree
    ${LIBNAME}
)
set_property(TARGET w3terrain_test PROPERTY CXX_STANDARD 23)

if(WIN32)
  add_custom_command(
    TARGET w3terrain_test
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:w3terrain_test> $<TARGET_FILE_DIR:w3terrain_test>
    COMMAND_EXPAND_LISTS)
endif()

include(GoogleTest)
gtest_discover_tests(w3terrain_test DISCOVERY_MODE PRE_TEST)
