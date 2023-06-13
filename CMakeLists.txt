
cmake_minimum_required(VERSION 3.15)
file(STRINGS "${CMAKE_SOURCE_DIR}/version.txt" projectVersion)
project("proxyfmu" VERSION ${projectVersion})
message("Current ${PROJECT_NAME} version: ${CMAKE_PROJECT_VERSION}\n")


# ==============================================================================
# Build settings
# ==============================================================================

set(BUILD_SHARED_LIBS OFF)
option(PROXYFMU_BUILD_TESTS "Build tests" OFF)
option(PROXYFMU_BUILD_EXAMPLES "Build examples" OFF)


# ==============================================================================
# Global internal configuration
# ==============================================================================

set(CMAKE_CXX_STANDARD 17)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(generatedSourcesDir "${CMAKE_BINARY_DIR}/generated")

if (MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_compile_options("/wd4996" "/wd4251" "/wd4244" "/wd4267")
endif ()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()


# ==============================================================================
# Dependencies
# ==============================================================================

set(SUBPROCESS_INCLUDE_DIR ${CMAKE_BINARY_DIR}/thirdparty)
file(DOWNLOAD https://raw.githubusercontent.com/Ecos-platform/subprocess.h/master/subprocess.h
        ${SUBPROCESS_INCLUDE_DIR}/subprocess/subprocess.h)

if (PROXYFMU_BUILD_TESTS)

    Include(FetchContent)

    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v2.13.8
    )

    FetchContent_MakeAvailable(Catch2)

endif ()

find_package(cli11 CONFIG REQUIRED)
find_package(thrift CONFIG REQUIRED)
find_package(fmilibcpp CONFIG REQUIRED)


# ==============================================================================
# Targets
# ==============================================================================

add_subdirectory(src)
add_subdirectory(tool)

if (PROXYFMU_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

if (PROXYFMU_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()


# ==============================================================================
# Exports and remaining installation
# ==============================================================================

configure_package_config_file(cmake/config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/proxyfmu-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/proxyfmu
        NO_SET_AND_CHECK_MACRO)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/proxyfmu-config-version.cmake
        VERSION ${CMAKE_PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)
install(
        FILES
        ${CMAKE_CURRENT_BINARY_DIR}/proxyfmu-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/proxyfmu-config-version.cmake
        DESTINATION
        ${CMAKE_INSTALL_DATADIR}/proxyfmu)

install(TARGETS proxyfmu proxyfmu-client EXPORT proxyfmu-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT proxyfmu-targets
        NAMESPACE proxyfmu::
        DESTINATION ${CMAKE_INSTALL_DATADIR}/proxyfmu)
