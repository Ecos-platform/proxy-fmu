
add_executable(proxyfmu_booter
        proxyfmu_booter.cpp
        boot_service_handler.cpp
        "$<TARGET_OBJECTS:proxyfmu-service>"
        "${generatedSourcesDir}/proxyfmu/lib_info.cpp")
target_include_directories(proxyfmu_booter
        PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${generatedSourcesDir}"
        "${SUBPROCESS_INCLUDE_DIR}"
        )
target_link_libraries(proxyfmu_booter
        PRIVATE
        CLI11::CLI11
        thrift::thrift
        )
if (UNIX)
    target_link_libraries(proxyfmu_booter PRIVATE
            pthread
            stdc++fs)
endif ()


# ==============================================================================
# Installation rules
# ==============================================================================

install(
        TARGETS
        proxyfmu_booter
        DESTINATION ${PROXYFMU_INSTALL_DESTINATIONS}
)
