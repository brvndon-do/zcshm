add_library(zcshm STATIC shm_region.cpp)

target_compile_features(zcshm PUBLIC cxx_std_20)
target_include_directories (zcshm PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(zcshm PUBLIC Threads::Threads)
target_link_libraries(zcshm PRIVATE zcshm_warnings)
