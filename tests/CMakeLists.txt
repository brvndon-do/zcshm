include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.18.0
)
FetchContent_MakeAvailable(googletest)

add_executable(zcshm_tests stress_test.cpp)
target_link_libraries(zcshm_tests PRIVATE zcshm zcshm_warnings gtest_main)

include(GoogleTest)
gtest_discover_tests(zcshm_tests)
