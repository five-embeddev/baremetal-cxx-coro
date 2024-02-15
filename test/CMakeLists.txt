cmake_minimum_required(VERSION 3.15)

include(ExternalProject)
set(UNITY unity_project)
ExternalProject_Add(
    unity_project
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/unity_project
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/unity_project ${MY_CMAKE_FLAGS}
)


ExternalProject_Get_Property(unity_project install_dir)
include_directories(${install_dir}/include/unity)

add_executable(unit_tests test_static_list.cpp test_timer_coro.cpp test_priority_coro.cpp unit_tests.cpp ../src/startup.cpp)

target_include_directories(unit_tests PRIVATE )
target_compile_features(unit_tests PUBLIC cxx_std_20)

add_dependencies(unit_tests unity_project)
target_link_libraries(unit_tests ${install_dir}/lib/libunity.a)

add_test(NAME unit_tests_run COMMAND $<TARGET_FILE:unit_tests>)
