add_subdirectory(thrift)

set(publicHeaderDir ${CMAKE_SOURCE_DIR}/include)
set(generatedSourcesDir "${CMAKE_BINARY_DIR}/generated")

# ==============================================================================
# common headers
# ==============================================================================

set(commonPublicHeaders

        "proxyfmu/fs_portability.hpp"
        "proxyfmu/lib_info.hpp"
        "proxyfmu/remote_info.hpp"

        )

set(commonPublicHeadersFull)
foreach (header IN LISTS commonPublicHeaders)
    list(APPEND commonPublicHeadersFull "${publicHeaderDir}/${header}")
endforeach ()


# Generate sources from .in files
set(libInfo "${generatedSourcesDir}/proxyfmu/lib_info.cpp")
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/proxyfmu/lib_info.cpp.in"
        "${libInfo}"
        @ONLY
)


# ==============================================================================
# proxyfmu client lib
# ==============================================================================

set(clientPublicHeaders
        "proxyfmu/client/proxy_fmu.hpp")

set(clientPrivateHeaders
        "proxyfmu/process_helper.hpp"
        "proxyfmu/client/proxy_slave.hpp")

set(clientSources
        "proxyfmu/client/proxy_slave.cpp"
        "proxyfmu/client/proxy_fmu.cpp")

set(clientPublicHeadersFull)
foreach (header IN LISTS clientPublicHeaders)
    list(APPEND clientPublicHeadersFull "${publicHeaderDir}/${header}")
endforeach ()

add_library(proxyfmu-client ${commonPublicHeadersFull} ${clientPublicHeadersFull} ${clientPrivateHeaders} ${clientSources} ${libInfo} "$<TARGET_OBJECTS:proxyfmu-service>")
add_library(proxyfmu::proxy-client ALIAS proxyfmu-client)
target_compile_features(proxyfmu-client PUBLIC "cxx_std_17")
set_target_properties(proxyfmu-client PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(proxyfmu-client
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${generatedSourcesDir}"
        )
target_link_libraries(proxyfmu-client
        PUBLIC
        fmilibcpp::fmilibcpp
        PRIVATE
        thrift::thrift
        Boost::filesystem
        )
if (UNIX)
    target_link_libraries(proxyfmu-client PRIVATE pthread)
endif ()

# Make sure executable is up to date
add_dependencies(proxyfmu-client proxyfmu)


# ==============================================================================
# Installation rules
# ==============================================================================

install(
        TARGETS proxyfmu-client
        EXPORT "${PROXYFMU_EXPORT_TARGET}"
        ${PROXYFMU_INSTALL_DESTINATIONS}
)
install(
        DIRECTORY "${publicHeaderDir}/proxyfmu"
        DESTINATION "${PROXYFMU_HEADER_INSTALL_DIR}"
)
