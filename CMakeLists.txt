
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
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

set(generatedSourcesDir "${CMAKE_BINARY_DIR}/generated")

if (MSVC)
    add_compile_options("/wd4996" "/wd4251" "/wd4244" "/wd4267")
endif ()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()

# Prepare for export and installation.
set(PROXYFMU_HEADER_INSTALL_DIR "include")
if (WIN32)
    set(PROXYFMU_CMAKE_INSTALL_DIR "cmake")
else ()
    set(PROXYFMU_CMAKE_INSTALL_DIR "share/${PROJECT_NAME}/cmake")
endif ()

set(PROXYFMU_INSTALL_DESTINATIONS
        ARCHIVE DESTINATION "lib"
        LIBRARY DESTINATION "lib"
        RUNTIME DESTINATION "bin"
        INCLUDES DESTINATION "${PROXYFMU_HEADER_INSTALL_DIR}")
set(PROXYFMU_EXPORT_TARGET "${PROJECT_NAME}-targets")


# ==============================================================================
# Dependencies
# ==============================================================================

if (CONAN_EXPORTED) # in conan local cache
    # standard conan installation, deps will be defined in conanfile.py
    # and not necessary to call conan again, conan is already running
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
else ()

    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan.cmake")
    conan_check(REQUIRED)

    conan_cmake_autodetect(settings)
    conan_add_remote(NAME ais URL https://ais.jfrog.io/artifactory/api/conan/ais-conan-local)

    conan_cmake_install(PATH_OR_REFERENCE ..
            BUILD missing
            SETTINGS ${settings})

    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()

endif ()

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

find_package(CLI11 REQUIRED)
find_package(Thrift REQUIRED)
find_package(Fmilibcpp CONFIG REQUIRED)


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

install(
        EXPORT "${PROXYFMU_EXPORT_TARGET}"
        DESTINATION "${PROXYFMU_CMAKE_INSTALL_DIR}"
        NAMESPACE "${PROJECT_NAME}::"
)

include(CMakePackageConfigHelpers)

# Generate and install package-config file.
set(configFile "${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config.cmake")
set(targetsFile "${PROXYFMU_CMAKE_INSTALL_DIR}/${PROXYFMU_EXPORT_TARGET}.cmake")
configure_package_config_file(
        "${CMAKE_SOURCE_DIR}/cmake/project-config.cmake.in"
        "${configFile}"
        INSTALL_DESTINATION "${PROXYFMU_CMAKE_INSTALL_DIR}"
        PATH_VARS targetsFile
)
install(FILES "${configFile}" DESTINATION "${PROXYFMU_CMAKE_INSTALL_DIR}")

# Generate and install package-version file
set(versionFile "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake")
write_basic_package_version_file(
        "${versionFile}"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY "SameMajorVersion")
install(FILES "${versionFile}" DESTINATION "${PROXYFMU_CMAKE_INSTALL_DIR}")
