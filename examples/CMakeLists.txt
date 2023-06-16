add_compile_definitions(PROXYFMU_DATA_DIR="${CMAKE_SOURCE_DIR}/data")

###########################################################
#########################FMI############################

add_executable(fmi-test fmi_test.cpp)
target_link_libraries(fmi-test PRIVATE fmilibcpp::fmilibcpp CLI11::CLI11)

###########################################################
#########################PROXY############################

add_executable(proxy-test proxy_test.cpp)
target_link_libraries(proxy-test PRIVATE proxyfmu-client CLI11::CLI11)
