
cmake_minimum_required(VERSION 3.25)

set(CMAKE_USER_MAKE_RULES_OVERRIDE "CMakeOverride.txt")

file(READ ${CMAKE_SOURCE_DIR}/vcpkg.json VCPKG_JSON_STRING)
string(JSON HIKOCSP_LIB_NAME     GET ${VCPKG_JSON_STRING} "name")
string(JSON HIKOCSP_LIB_VERSION  GET ${VCPKG_JSON_STRING} "version")
string(JSON HIKOCSP_LIB_LICENSE  GET ${VCPKG_JSON_STRING} "license")
string(JSON HIKOCSP_LIB_DESC     GET ${VCPKG_JSON_STRING} "description")
string(JSON HIKOCSP_LIB_HOMEPAGE GET ${VCPKG_JSON_STRING} "homepage")

project(${HIKOCSP_LIB_NAME} VERSION ${HIKOCSP_LIB_VERSION} LANGUAGES CXX)

include(FetchContent)

# disable adding CTest build targets like "NightlyStart" (before ctest include)
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
include(CTest)

if(BUILD_TESTING)
    #
    # GoogleTest - non-vcpkg, directly build from externals
    #
    set(INSTALL_GTEST OFF CACHE INTERNAL "Don't install gtest")
    set(BUILD_GMOCK OFF CACHE INTERNAL "Don't build gmock")
    FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.13.0)
    FetchContent_MakeAvailable(googletest)

    include(GoogleTest)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_target_properties(gtest      PROPERTIES FOLDER extern)
    set_target_properties(gtest_main PROPERTIES FOLDER extern)
endif()

set(HIKOCSP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/hikocsp")
add_library(hikocsp INTERFACE)
set_target_properties(hikocsp PROPERTIES DEBUG_POSTFIX "d")
target_compile_features(hikocsp INTERFACE cxx_std_20)
target_compile_options(hikocsp INTERFACE -arch:AVX2)

if (MSVC)
    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
        # Just my code enables the VS debugger to step over system, framework, library, and other non-user calls.
        target_compile_options(hikocsp INTERFACE -JMC)
    endif()
endif()

target_sources(hikocsp PUBLIC FILE_SET hikocsp_include_files TYPE HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/" FILES
    ${HIKOCSP_SOURCE_DIR}/csp_parser.hpp
    ${HIKOCSP_SOURCE_DIR}/generator.hpp
)

target_include_directories(hikocsp INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:include>"
)

add_executable(hikocsp-bin main.cpp)
target_link_libraries(hikocsp-bin hikocsp)
set_target_properties(hikocsp-bin PROPERTIES OUTPUT_NAME hikocsp)

if(BUILD_TESTING)
    add_executable(hikocsp_tests)
    target_link_libraries(hikocsp_tests PRIVATE gtest_main hikocsp)
    target_include_directories(hikocsp_tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

    target_sources(hikocsp_tests PRIVATE
        ${HIKOCSP_SOURCE_DIR}/csp_parser_tests.cpp
        ${HIKOCSP_SOURCE_DIR}/generator_tests.cpp
    )
    gtest_discover_tests(hikocsp_tests DISCOVERY_MODE PRE_TEST)
endif()
