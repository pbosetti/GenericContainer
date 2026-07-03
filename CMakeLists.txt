############################################################################
#                                                                          #
#  file: CMakeLists.txt                                                    #
#  Copyright (C) 2016                                                      #
#      Enrico Bertolazzi                                                   #
#      Dipartimento di Ingegneria Industriale                              #
#      Università degli Studi di Trento                                    #
#      email: enrico.bertolazzi@unitn.it                                   #
#                                                                          #
############################################################################

cmake_minimum_required(VERSION 3.14)

project( GenericContainer )

# cmake_utils is a scripts-only repo (no top-level CMakeLists.txt), so
# FetchContent_MakeAvailable() just downloads/populates it without trying to
# add_subdirectory() it. Pinned to a commit since upstream publishes no tags;
# override locally for development with -DFETCHCONTENT_SOURCE_DIR_CMAKE_UTILS=<path>.
include( FetchContent )
# CMP0135 (extract URL-fetched archives with the extraction time as the timestamp,
# rather than preserving the timestamp recorded in the archive) only exists from
# CMake 3.24 on; guard it so older CMake (e.g. the 3.22 shipped by Ubuntu 22.04) does
# not fail with "Policy CMP0135 is not known".
if ( POLICY CMP0135 )
  cmake_policy( SET CMP0135 NEW )
endif()
FetchContent_Declare(
  cmake_utils
  GIT_REPOSITORY https://github.com/ebertolazzi/cmake_utils.git
  GIT_TAG        e8c50a2eda739e569dde5d2851ac713e28ddc9f3
)
FetchContent_MakeAvailable( cmake_utils )
set( HOME "${cmake_utils_SOURCE_DIR}/" )

include( "${HOME}/CMakeLists-common.txt" )

project(
  ${PROJECT_NAME}
  VERSION ${UTILS_PROJECT_VERSION}
  HOMEPAGE_URL "https://ebertolazzi.github.io/GenericContainer/"
)

include( "${HOME}/CMakeLists-cflags.txt" )
include( "${HOME}/CMakeLists-utilities.txt" )

# CMakeLists-cflags.txt hardcodes "-std=c++17" into CMAKE_CXX_FLAGS; scrub it so the
# cxx_std_20 usage requirement below is the single source of truth for the standard
# (with both present, the last -std flag on the command line would win anyway, but
# only by accident of ordering).
string( REPLACE "-std=c++17" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )

# Eigen dense matrices back the mat_* types, so Eigen is a PUBLIC dependency of
# the library. Fetched headers-only (Eigen's own CMakeLists would configure its
# test/doc machinery), same hand-made INTERFACE-target pattern used for
# nlohmann_json below; SYSTEM keeps Eigen's internals out of our warning set.
FetchContent_Declare(
  eigen3
  URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
)
FetchContent_GetProperties( eigen3 )
if( NOT eigen3_POPULATED )
  FetchContent_Populate( eigen3 )
endif()
add_library( Eigen3_Eigen INTERFACE )
add_library( Eigen3::Eigen ALIAS Eigen3_Eigen )
target_include_directories(
  Eigen3_Eigen SYSTEM INTERFACE
  "$<BUILD_INTERFACE:${eigen3_SOURCE_DIR}>"
)

# Evaluating the suffix to append to target built by this CMake file
utils_artifacts_suffix(ARTIFACTS_STATIC_SUFFIX TRUE)
if( UTILS_BUILD_SHARED )
  utils_artifacts_suffix(ARTIFACTS_DYNAMIC_SUFFIX FALSE)
endif()

if( CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang" )
  add_compile_options( "$<$<COMPILE_LANGUAGE:C,CXX>:-Wno-poison-system-directories>" )
  add_compile_options( "$<$<COMPILE_LANGUAGE:C,CXX>:-Wno-switch-default>" )
  add_compile_options( "$<$<COMPILE_LANGUAGE:C,CXX>:-Wno-exit-time-destructors>" )
endif()

message( STATUS "Compiler used: ${CMAKE_CXX_COMPILER_ID}" )
message( STATUS "BASE SUFFIXES = ${ARTIFACTS_STATIC_SUFFIX} ${ARTIFACTS_DYNAMIC_SUFFIX}" )

if( EXISTS "${CMAKE_CURRENT_LIST_DIR}/../CMakeLists-customize.txt" )
  include(${CMAKE_CURRENT_LIST_DIR}/../CMakeLists-customize.txt)
endif()

if (APPLE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLUA_USE_MACOSX")
elseif(LINUX OR UNIX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLUA_USE_LINUX")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif()

set(LUALIB Lua)

#   ___
#  / __| ___ _  _ _ _ __ ___ ___
#  \__ \/ _ \ || | '_/ _/ -_|_-<
#  |___/\___/\_,_|_| \__\___/__/
#
set( SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" )

set( SOURCES )
file(
  GLOB S
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src_yaml_interface/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src_json_interface/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src_toml_interface/*.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src_lua_interface/*.cc
)
foreach(F ${S})
  # build relative path respect to source directory
  file( RELATIVE_PATH RF ${CMAKE_CURRENT_SOURCE_DIR} "${F}" )
  list( APPEND SOURCES ${RF} )
endforeach(F ${S})


set( SOURCES_EXTRA )
file(
  GLOB S
)
foreach(F ${S})
  # build relative path respect to source directory
  file( RELATIVE_PATH RF ${CMAKE_CURRENT_SOURCE_DIR} "${F}" )
  list( APPEND SOURCES_EXTRA ${RF} )
  list( APPEND SOURCES       ${RF} )
endforeach(F ${S})

#list(FILTER SOURCES EXCLUDE REGEX ".*hsl_fake.cc$")

#   _  _             _
#  | || |___ __ _ __| |___ _ _ ___
#  | __ / -_) _` / _` / -_) '_(_-<
#  |_||_\___\__,_\__,_\___|_| /__/
#
set( PUBLIC_HEADERS )
file(
  GLOB S
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h*
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*/*.h*
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*/*/*.h*
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*/*/*/*.h*
)
foreach (F ${S})
  # build relative path respect to source directory
  file( RELATIVE_PATH RF "${CMAKE_CURRENT_SOURCE_DIR}/include" "${F}" )
  list( APPEND PUBLIC_HEADERS ${RF} )
endforeach(F ${S})

# extra include directory for compilation
include_directories( ./include ./src_yaml_interface ./src_json_interface )

#  _    _   _   _
# | |  | | | | / \
# | |  | | | |/ _ \
# | |__| |_| / ___ \
# |_____\___/_/   \_\
#
# LUA LIBRARY
set( lualib_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src_lua_interface/lua-5.4.6/src/" )

set( lualib_HEADERS )
file( GLOB S "${lualib_DIR}/lua*.h*" "${lualib_DIR}/lauxlib.h" )
foreach (F ${S})
  # build relative path respect to source directory
  file( RELATIVE_PATH RF "${lualib_DIR}" "${F}" )
  list( APPEND lualib_HEADERS ${RF} )
endforeach(F ${S})

set( lualib_SOURCES )
file( GLOB S "${lualib_DIR}/*.c" )
foreach (F ${S})
  # build relative path respect to source directory
  file( RELATIVE_PATH RF "${lualib_DIR}" "${F}" )
  list( APPEND lualib_SOURCES ${RF} )
endforeach(F ${S})

list( FILTER lualib_SOURCES EXCLUDE REGEX ".*luac?\.c$" )

#   _____                  _
#  |_   _|_ _ _ _ __ _ ___| |_ ___
#    | |/ _` | '_/ _` / -_)  _(_-<
#    |_|\__,_|_| \__, \___|\__/__/
#                |___/
#
# define target
utils_setup_target(
  Lua
  TRUE # build static
  "${lualib_DIR}"
  "${lualib_SOURCES}"
  "${lualib_DIR}"
  "${lualib_HEADERS}"
)
utils_setup_target(
  ${PROJECT_NAME}
  TRUE # build static
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${SOURCES}"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${PUBLIC_HEADERS}"
)
# utils_setup_target() only publishes CMAKE_CURRENT_SOURCE_DIR (the repo root) as a
# PUBLIC include dir, not the "include" subfolder where the public headers actually
# live, so downstream consumers (e.g. via FetchContent) cannot find <GenericContainer.hh>.
target_include_directories(
  "${UTILS_NAMESPACE}_${PROJECT_NAME}_Static" PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
# The -std flag injected by CMakeLists-cflags.txt is scrubbed above and is scoped to
# this directory anyway, so require C++20 via a PUBLIC usage requirement to propagate
# it to anything linking this library.
target_compile_features( "${UTILS_NAMESPACE}_${PROJECT_NAME}_Static" PUBLIC cxx_std_20 )
target_link_libraries( "${UTILS_NAMESPACE}_${PROJECT_NAME}_Static" PUBLIC Eigen3::Eigen )
if ( UTILS_BUILD_SHARED )
  utils_setup_target(
    Lua
    FALSE # build dynamic
    "${lualib_DIR}"
    "${lualib_SOURCES}"
    "${lualib_DIR}"
    "${lualib_HEADERS}"
  )
  utils_setup_target(
    ${PROJECT_NAME}
    FALSE # build dynamic
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${SOURCES}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${PUBLIC_HEADERS}"
  )
  target_include_directories(
    "${UTILS_NAMESPACE}_${PROJECT_NAME}" PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  )
  target_compile_features( "${UTILS_NAMESPACE}_${PROJECT_NAME}" PUBLIC cxx_std_20 )
  target_link_libraries( "${UTILS_NAMESPACE}_${PROJECT_NAME}" PUBLIC Eigen3::Eigen "${UTILS_NAMESPACE}_Lua" )
endif()

if ( UNIX )
  set_property(SOURCE ${SOURCES_EXTRA} PROPERTY COMPILE_FLAGS "-Wno-exit-time-destructors -Wno-suggest-destructor-override -Wno-deprecated-copy-with-dtor -Wno-missing-prototypes -Wno-covered-switch-default -Wno-unreachable-code-return -Wno-missing-noreturn -Wno-shadow" )
endif()

#   _____       _
#  |_   _|__ __| |_ ___
#    | |/ -_|_-<  _(_-<
#    |_|\___/__/\__/__/
#
# Only build examples/tests when GenericContainer is the top-level project: when it is
# pulled in via add_subdirectory/FetchContent, these targets would collide with targets
# of the same generic name (test_all, example01, ...) defined by sibling dependencies,
# get added to the consumer's default ALL target, and write executables into this
# source tree instead of the consumer's build tree.
if ( UTILS_ENABLE_TESTS AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR )

  enable_testing()

  # nlohmann/json is only needed to build this project's own examples/tests exercising
  # include/GenericContainer/GenericContainerInterface_nlohmann.hh (an opt-in header);
  # GenericContainer itself has no hard dependency on it. Fetched as the "include.zip"
  # release asset (headers only, no CMakeLists.txt) rather than the git repo, which is
  # huge (full test suite + history), so the nlohmann_json::nlohmann_json target is
  # defined by hand below.
  FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/include.zip
  )
  FetchContent_MakeAvailable( nlohmann_json )
  add_library( nlohmann_json INTERFACE )
  add_library( nlohmann_json::nlohmann_json ALIAS nlohmann_json )
  target_include_directories(
    nlohmann_json INTERFACE
    "$<BUILD_INTERFACE:${nlohmann_json_SOURCE_DIR}/include>"
  )

  # Expressionist evaluates algebraic expressions embedded as tagged strings ("$...")
  # in nlohmann::json fields -- how we'll solve math expressions possibly stored in
  # GenericContainer's JSON. It's a small single-header repo (unlike nlohmann/json,
  # no huge history/test data), so a plain git fetch is fine here; its own
  # CMakeLists.txt already defines Expressionist::Expressionist (requires C++20,
  # which CMake propagates only to whatever links it) and fetches nlohmann_json
  # itself, reusing the nlohmann_json::nlohmann_json target declared above instead of
  # populating a second copy. Pinned to a commit since upstream publishes no tags.
  FetchContent_Declare(
    Expressionist
    GIT_REPOSITORY https://github.com/pbosetti/Expressionist.git
    GIT_TAG        fd5479eec349047edc97fd7fe42d24a21f749912
  )
  FetchContent_MakeAvailable( Expressionist )

  add_subdirectory( tests )

  set( EXELISTBIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin" )
  set( EXELISTCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src_tests" )

  set(
    EXELISTCPP
    test_all
    example01 example02 example03 example04 example05
    example07 example08 example09 example10 example11
    example12 example13 example14 example15 example16
    example17 example18
  )

  set( EXELISTC example06 )
  set( EXEJSONTEST )
  set( EXELUATEST example_lua1 example_lua2 example_lua3 )

  add_custom_target( "${PROJECT_NAME}_all_tests" ALL )

  set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin )
  set( LIBS ${UTILS_NAMESPACE}_${PROJECT_NAME}_Static ${UTILS_NAMESPACE}_Lua_Static ${CMAKE_DL_LIBS} nlohmann_json::nlohmann_json )

  foreach( S ${EXELISTCPP} )
    add_executable( ${S} ${CMAKE_CURRENT_SOURCE_DIR}/examples/${S}.cc )
    target_link_libraries( ${S} ${LIBS} )
    set_target_properties( ${S} PROPERTIES SUFFIX ".exe" )
    add_test( NAME "${S}" COMMAND ./bin/${S}.exe WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
    add_dependencies( "${PROJECT_NAME}_all_tests" ${S} )
    set_target_properties( ${S} PROPERTIES XCODE_GENERATE_SCHEME TRUE XCODE_SCHEME_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
  endforeach()
  foreach( S ${EXELISTC} )
    add_executable( ${S} ${CMAKE_CURRENT_SOURCE_DIR}/examples/${S}.c )
    target_link_libraries( ${S} ${LIBS} )
    set_target_properties( ${S} PROPERTIES SUFFIX ".exe" )
    add_test( NAME "${S}" COMMAND ./bin/${S}.exe WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
    add_dependencies( "${PROJECT_NAME}_all_tests" ${S} )
    set_target_properties( ${S} PROPERTIES XCODE_GENERATE_SCHEME TRUE XCODE_SCHEME_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
  endforeach()
  foreach( S ${EXEJSONTEST} )
    add_executable( ${S} ${CMAKE_CURRENT_SOURCE_DIR}/src_json_interface/tests/${S}.cc )
    target_link_libraries( ${S} ${LIBS} )
    set_target_properties( ${S} PROPERTIES SUFFIX ".exe" )
    add_test( NAME "${S}" COMMAND ./bin/${S}.exe WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
    add_dependencies( "${PROJECT_NAME}_all_tests" ${S} )
    set_target_properties( ${S} PROPERTIES XCODE_GENERATE_SCHEME TRUE XCODE_SCHEME_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
  endforeach()
  foreach( S ${EXELUATEST} )
    add_executable( ${S} ${CMAKE_CURRENT_SOURCE_DIR}/src_lua_interface/tests/${S}.cc )
    target_link_libraries( ${S} ${LIBS} )
    set_target_properties( ${S} PROPERTIES SUFFIX ".exe" )
    add_test( NAME "${S}" COMMAND ./bin/${S}.exe WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
    add_dependencies( "${PROJECT_NAME}_all_tests" ${S} )
    set_target_properties( ${S} PROPERTIES XCODE_GENERATE_SCHEME TRUE XCODE_SCHEME_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
  endforeach()

endif()

#   ___         _        _ _
#  |_ _|_ _  __| |_ __ _| | |
#   | || ' \(_-<  _/ _` | | |
#  |___|_||_/__/\__\__,_|_|_|
#
install(
  TARGETS ${TARGETS_TO_BE_INSTALLED}
  # EXPORT (da definire)
  LIBRARY DESTINATION ${INSTALL_PREFIX}/dll
  ARCHIVE DESTINATION ${INSTALL_PREFIX}/lib
  RUNTIME DESTINATION ${INSTALL_PREFIX}/bin
  PUBLIC_HEADER DESTINATION ${PROJECT_PUBLIC_HEADER_DESTINATION}
)

#   ___         _
#  | _ \__ _ __| |____ _ __ _ ___
#  |  _/ _` / _| / / _` / _` / -_)
#  |_| \__,_\__|_\_\__,_\__, \___|
#                       |___/
#

# do not work
#include( "${HOME}/CMakeLists-cpack.txt" )

utils_final_messages()

#
# Remove warning for 3rd
#
get_property(all_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)

foreach(target IN LISTS all_targets)
  if (TARGET ${target})
    get_target_property(type ${target} TYPE)
    if(type STREQUAL "EXECUTABLE" OR
       type STREQUAL "STATIC_LIBRARY" OR
       type STREQUAL "SHARED_LIBRARY" OR
       type STREQUAL "MODULE_LIBRARY" OR
       type STREQUAL "OBJECT_LIBRARY")
      target_include_directories(${target} SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/lib/include/Utils/3rd)
      target_include_directories(${target} SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/src_yaml_interface/)
      target_include_directories(${target} SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/src_lua_interface/)
    endif()
  endif()
endforeach()
